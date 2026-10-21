#include "ui/controllers/library_controller.h"

#include "library/audio_formats.h"
#include "library/natural_sort.h"
#include "ui/controllers/app_settings.h"
#include "ui/controllers/playlist_controller.h"
#include "ui/controllers/player_controller.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>

namespace resonance {

LibraryController::LibraryController(PlayerController *player,
                                     AppSettings *settings,
                                     PlaylistController *playlists,
                                     QObject *parent)
    : QObject(parent)
    , m_player(player)
    , m_settings(settings)
    , m_playlists(playlists)
{
    if (m_player)
        m_player->setTrackModel(&m_tracks);

    connect(&m_scanner, &FolderScanner::scanStarted, this,
            [this](const QString &root) {
                const bool folderDidChange = m_folder != root;
                m_rootFolder = root;
                m_folder = root;
                if (folderDidChange)
                    emit folderChanged();

                m_allFiles.clear();
                m_currentFiles.clear();
                m_tracks.clear();
                m_folders.clear();
                m_metadataLoader.start({});
                emit tracksChanged();
            });

    connect(&m_scanner, &FolderScanner::scanFinished, this,
            &LibraryController::handleScanFinished);
    connect(&m_scanner, &FolderScanner::scanFailed, this,
            &LibraryController::handleScanFailed);
    connect(&m_scanner, &FolderScanner::scanningChanged, this,
            &LibraryController::scanningChanged);

    connect(&m_metadataLoader, &MetadataLoader::batchReady, this,
            [this](quint64, const QVector<TrackMetadata> &batch) {
                for (const TrackMetadata &item : batch)
                    m_metadataCache.insert(item.path, item);
                m_tracks.applyMetadata(batch);
                if (m_player)
                    m_player->refreshTrackInfo();
            });

    if (m_playlists) {
        connect(m_playlists, &PlaylistController::playlistTracksChanged, this,
                [this](const QString &id) {
                    if (m_playlistMode && id == m_activePlaylistId)
                        reloadActivePlaylist();
                });
        connect(m_playlists, &PlaylistController::playlistRenamed, this,
                [this](const QString &id) {
                    if (m_playlistMode && id == m_activePlaylistId)
                        emit viewChanged();
                });
        connect(m_playlists, &PlaylistController::playlistRemoved, this,
                [this](const QString &id) {
                    if (!m_playlistMode || id != m_activePlaylistId)
                        return;
                    leavePlaylistMode();
                    if (m_player) {
                        m_player->stop();
                        m_player->setQueue({}, 0, 0, false);
                    }
                    if (!m_allFiles.isEmpty())
                        setSelectedFolder(m_folder, false);
                    else {
                        m_currentFiles.clear();
                        m_tracks.clear();
                        m_metadataLoader.start({});
                        emit tracksChanged();
                    }
                });
    }
}

QString LibraryController::folderName() const
{
    if (m_folder.isEmpty())
        return tr("No folder open");

    const QString name = QFileInfo(m_folder).fileName();
    return name.isEmpty() ? m_folder : name;
}

QString LibraryController::viewTitle() const
{
    if (m_playlistMode && m_playlists) {
        const QString name = m_playlists->name(m_activePlaylistId);
        if (!name.isEmpty())
            return name;
        return tr("Playlist");
    }
    return folderName();
}

QString LibraryController::viewSubtitle() const
{
    if (m_playlistMode)
        return tr("%1 tracks · playlist").arg(m_tracks.count());
    return m_folder;
}

void LibraryController::leavePlaylistMode()
{
    if (!m_playlistMode && m_activePlaylistId.isEmpty())
        return;

    m_playlistMode = false;
    m_activePlaylistId.clear();
    if (m_playlists)
        m_playlists->setCurrent({});
    if (m_settings) {
        m_settings->setLastMode(QStringLiteral("folder"));
        m_settings->setLastPlaylistId({});
    }
    emit viewChanged();
}

void LibraryController::setStartupFolder(const QString &path, bool autoPlay)
{
    m_startupFolder = path;
    m_startupAutoPlay = autoPlay;
}

void LibraryController::openFolder(const QUrl &url)
{
    if (!url.isLocalFile()) {
        setError(tr("Only local folders are supported."));
        return;
    }
    openFolderPath(url.toLocalFile(), true);
}

void LibraryController::openFolderPath(const QString &path, bool autoPlay)
{
    clearError();
    leavePlaylistMode();
    m_restoreRequested = false;
    m_refreshRequested = false;
    m_openAutoPlay = autoPlay;

    if (m_player) {
        m_player->stop();
        m_player->setQueue({}, 0, 0, false);
    }

    m_scanner.scanPath(path);
}

void LibraryController::openUrls(const QList<QUrl> &urls)
{
    clearError();

    QList<QUrl> localUrls;
    for (const QUrl &url : urls) {
        if (url.isLocalFile())
            localUrls.append(url);
    }

    if (localUrls.isEmpty()) {
        setError(tr("Only local files and folders are supported."));
        return;
    }

    for (const QUrl &url : localUrls) {
        if (QFileInfo(url.toLocalFile()).isDir()) {
            openFolder(url);
            return;
        }
    }

    QStringList files;
    for (const QUrl &url : localUrls) {
        const QString path = url.toLocalFile();
        if (isSupportedAudioPath(path))
            files.append(path);
    }

    if (files.isEmpty()) {
        setError(tr("No supported audio files were dropped."));
        return;
    }

    std::sort(files.begin(), files.end(),
              [](const QString &left, const QString &right) {
                  return naturalLess(left, right);
              });

    const QString root = QFileInfo(files.first()).absolutePath();
    m_restoreRequested = false;
    m_refreshRequested = false;
    m_openAutoPlay = true;

    if (m_player) {
        m_player->stop();
        m_player->setQueue({}, 0, 0, false);
    }

    m_rootFolder = root;
    m_folder = root;
    m_allFiles = files;
    m_folders.setRoot(root, m_allFiles);
    setSelectedFolder(root, false);
    if (m_settings)
        m_settings->setLastFolder(root);

    if (m_player && !m_currentFiles.isEmpty())
        m_player->setQueue(m_currentFiles, 0, 0, true);

}

void LibraryController::playFolder(const QString &folder)
{
    if (m_allFiles.isEmpty())
        return;

    leavePlaylistMode();
    clearError();
    setSelectedFolder(folder, false);

    if (m_currentFiles.isEmpty()) {
        setError(tr("No supported audio files were found in this folder."));
        if (m_player)
            m_player->setQueue({}, 0, 0, false);
        return;
    }

    if (m_player)
        m_player->setQueue(m_currentFiles, 0, 0, true);
}

void LibraryController::playPlaylist(const QString &id)
{
    if (!m_playlists || id.isEmpty())
        return;

    const QStringList tracks = m_playlists->tracks(id);
    showPlaylist(id, tracks, 0, 0, true);
}

void LibraryController::showPlaylist(const QString &id, const QStringList &tracks,
                                     int startIndex, qint64 positionMs,
                                     bool autoPlay)
{
    if (!m_playlists)
        return;

    m_playlistMode = true;
    m_activePlaylistId = id;
    m_playlists->setCurrent(id);

    m_currentFiles = tracks;
    m_tracks.setFiles(QString(), tracks);
    applyCachedMetadata();
    startMetadataLoad(tracks);

    if (m_player) {
        if (tracks.isEmpty()) {
            m_player->stop();
            m_player->setQueue({}, 0, 0, false);
        } else {
            const int index = std::clamp(startIndex, 0,
                                         static_cast<int>(tracks.size()) - 1);
            m_player->setQueue(tracks, index, positionMs, autoPlay);
        }
    }

    if (m_settings) {
        m_settings->setLastMode(QStringLiteral("playlist"));
        m_settings->setLastPlaylistId(id);
    }

    emit viewChanged();
    emit tracksChanged();
}

void LibraryController::reloadActivePlaylist()
{
    if (!m_playlistMode || !m_playlists || m_activePlaylistId.isEmpty())
        return;

    const QStringList tracks = m_playlists->tracks(m_activePlaylistId);
    if (tracks.isEmpty()) {
        m_currentFiles.clear();
        m_tracks.clear();
        m_metadataLoader.start({});
        if (m_player) {
            m_player->stop();
            m_player->setQueue({}, 0, 0, false);
        }
        emit viewChanged();
        emit tracksChanged();
        return;
    }

    const QString currentPath = m_player ? m_player->path() : QString();
    const int currentIndex = tracks.indexOf(currentPath);
    const int index = currentIndex >= 0 ? currentIndex : 0;
    const qint64 position = currentIndex >= 0
        ? (m_player ? m_player->positionMs() : 0) : 0;
    const bool wasPlaying = m_player ? m_player->playing() : false;

    m_currentFiles = tracks;
    m_tracks.setFiles(QString(), tracks);
    applyCachedMetadata();
    startMetadataLoad(tracks);

    if (m_player)
        m_player->setQueue(tracks, index, position, wasPlaying);

    if (m_settings) {
        m_settings->setLastMode(QStringLiteral("playlist"));
        m_settings->setLastPlaylistId(m_activePlaylistId);
    }

    emit viewChanged();
    emit tracksChanged();
}

void LibraryController::playIndex(int index)
{
    if (index < 0 || index >= m_currentFiles.size() || !m_player)
        return;

    m_player->setQueue(m_currentFiles, index, 0, true);
}

void LibraryController::playNext(int index)
{
    if (index < 0 || index >= m_currentFiles.size() || !m_player)
        return;
    m_player->playNext(m_currentFiles.at(index));
}

void LibraryController::addCurrentToPlaylist(const QString &id)
{
    if (!m_playlists || id.isEmpty())
        return;
    if (m_playlistMode) {
        setError(tr("Switch to a folder view before adding a folder."));
        return;
    }

    QStringList files = m_currentFiles;
    if (files.isEmpty())
        files = m_allFiles;
    if (files.isEmpty()) {
        setError(tr("There are no tracks to add."));
        return;
    }
    m_playlists->addTracks(id, files);
}

void LibraryController::playFromIndex(int index)
{
    playIndex(index);
}

void LibraryController::nextFolder()
{
    if (m_playlistMode)
        return;
    const QString next = m_folders.nextFolderPath(m_folder);
    if (!next.isEmpty())
        playFolder(next);
}

void LibraryController::previousFolder()
{
    if (m_playlistMode)
        return;
    const QString previous = m_folders.previousFolderPath(m_folder);
    if (!previous.isEmpty())
        playFolder(previous);
}

void LibraryController::restoreLastSession()
{
    clearError();
    m_restoreRequested = true;
    m_refreshRequested = false;
    m_openAutoPlay = false;

    if (m_settings && m_playlists
        && m_settings->lastMode() == QStringLiteral("playlist")) {
        const QString id = m_settings->lastPlaylistId();
        const QStringList tracks = m_playlists->tracks(id);
        if (!tracks.isEmpty()) {
            int index = tracks.indexOf(m_settings->lastTrack());
            if (index < 0)
                index = 0;
            const qint64 position = index >= 0
                && tracks.at(index) == m_settings->lastTrack()
                ? m_settings->lastPositionMs() : 0;
            showPlaylist(id, tracks, index, position, false);
            m_restoreRequested = false;
            return;
        }
    }

    const QString lastFolder = m_settings ? m_settings->lastFolder() : QString();
    if (lastFolder.isEmpty()) {
        m_restoreRequested = false;
        return;
    }
    m_scanner.scanPath(lastFolder);
}

void LibraryController::refresh()
{
    if (m_playlistMode) {
        reloadActivePlaylist();
        return;
    }

    if (m_rootFolder.isEmpty()) {
        setError(tr("Open a folder first."));
        return;
    }

    m_restoreRequested = false;
    m_refreshRequested = true;
    m_openAutoPlay = false;
    m_refreshFolder = m_folder;
    m_refreshTrack = m_player ? m_player->path() : QString();
    m_refreshPosition = m_player ? m_player->positionMs() : 0;
    m_refreshAutoPlay = m_player ? m_player->playing() : false;
    m_scanner.scanPath(m_rootFolder);
}

void LibraryController::clearError()
{
    setError({});
}

void LibraryController::handleScanFinished(const QString &root,
                                           const QStringList &files)
{
    m_rootFolder = root;
    m_folder = root;
    if (m_settings)
        m_settings->setLastFolder(root);

    m_allFiles = files;
    m_folders.setRoot(root, files);

    if (files.isEmpty()) {
        setSelectedFolder(root, false);
        if (m_player)
            m_player->setQueue({}, 0, 0, false);
        setError(tr("No supported audio files were found in this folder."));
        m_openAutoPlay = true;
        m_restoreRequested = false;
        m_refreshRequested = false;
        emit folderChanged();
        return;
    }

    if (m_restoreRequested) {
        QString selected = m_settings
            ? m_settings->lastSelectedFolder() : QString();
        const QString lastTrack = m_settings ? m_settings->lastTrack() : QString();

        if (!lastTrack.isEmpty()) {
            const QString trackFolder = QFileInfo(lastTrack).absolutePath();
            if (!filesUnder(trackFolder).contains(lastTrack))
                selected = root;
            else
                selected = trackFolder;
        }

        if (selected.isEmpty() || filesUnder(selected).isEmpty())
            selected = root;

        setSelectedFolder(selected, false);

        const int index = m_currentFiles.indexOf(lastTrack);
        if (m_player) {
            if (index >= 0) {
                m_player->setQueue(m_currentFiles, index,
                                   m_settings->lastPositionMs(), false);
            } else {
                m_player->setQueue({}, 0, 0, false);
            }
        }

        m_restoreRequested = false;
    } else if (m_refreshRequested) {
        const QString selected = filesUnder(m_refreshFolder).isEmpty()
            ? root : m_refreshFolder;
        setSelectedFolder(selected, false);

        const int index = m_currentFiles.indexOf(m_refreshTrack);
        if (m_player) {
            if (index >= 0) {
                m_player->setQueue(m_currentFiles, index,
                                   m_refreshPosition, m_refreshAutoPlay);
            } else {
                m_player->setQueue({}, 0, 0, false);
            }
        }
        m_refreshRequested = false;
        m_refreshTrack.clear();
        m_refreshFolder.clear();
    } else {
        setSelectedFolder(root, false);
        if (m_player && m_openAutoPlay)
            m_player->setQueue(m_currentFiles, 0, 0, true);
        m_openAutoPlay = true;
    }

    setError({});
    emit folderChanged();
}

void LibraryController::handleScanFailed(const QString &message)
{
    setError(message);
    m_openAutoPlay = true;
    m_restoreRequested = false;
    m_refreshRequested = false;
}


void LibraryController::setSelectedFolder(const QString &folder, bool autoPlay)
{
    leavePlaylistMode();

    QString target = folder;
    if (target.isEmpty() || filesUnder(target).isEmpty())
        target = m_rootFolder;

    m_folder = target;
    m_currentFiles = filesUnder(target);

    m_folders.setCurrentFolder(target);
    m_tracks.setFiles(target, m_currentFiles);
    applyCachedMetadata();
    startMetadataLoad(m_currentFiles);

    if (m_settings)
        m_settings->setLastSelectedFolder(target);

    emit folderChanged();
    emit tracksChanged();

    if (autoPlay && m_player) {
        if (m_currentFiles.isEmpty())
            m_player->setQueue({}, 0, 0, false);
        else
            m_player->setQueue(m_currentFiles, 0, 0, true);
    }
}

void LibraryController::setError(const QString &message)
{
    if (m_error == message)
        return;
    m_error = message;
    emit errorChanged();
}

void LibraryController::applyCachedMetadata()
{
    if (m_currentFiles.isEmpty() || m_metadataCache.isEmpty())
        return;

    QVector<TrackMetadata> batch;
    batch.reserve(m_currentFiles.size());
    for (const QString &path : m_currentFiles) {
        const auto found = m_metadataCache.constFind(path);
        if (found != m_metadataCache.constEnd())
            batch.append(found.value());
    }

    if (!batch.isEmpty()) {
        m_tracks.applyMetadata(batch);
        if (m_player)
            m_player->refreshTrackInfo();
    }
}

void LibraryController::startMetadataLoad(const QStringList &files)
{
    m_metadataLoader.start(files);
}

QStringList LibraryController::filesUnder(const QString &folder) const
{
    if (folder.isEmpty())
        return {};

    if (folder == m_rootFolder)
        return m_allFiles;

    const QDir directory(folder);
    QStringList result;
    for (const QString &file : m_allFiles) {
        const QString relative = directory.relativeFilePath(file);
        if (!QDir::isAbsolutePath(relative)
            && relative != QStringLiteral("..")
            && !relative.startsWith(QStringLiteral("../"))) {
            result.append(file);
        }
    }
    return result;
}

} // namespace resonance
