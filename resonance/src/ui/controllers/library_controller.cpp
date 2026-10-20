#include "ui/controllers/library_controller.h"

#include "library/audio_formats.h"
#include "library/natural_sort.h"
#include "ui/controllers/app_settings.h"
#include "ui/controllers/player_controller.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>

namespace resonance {

LibraryController::LibraryController(PlayerController *player,
                                     AppSettings *settings, QObject *parent)
    : QObject(parent)
    , m_player(player)
    , m_settings(settings)
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
}

QString LibraryController::folderName() const
{
    if (m_folder.isEmpty())
        return tr("No folder open");

    const QString name = QFileInfo(m_folder).fileName();
    return name.isEmpty() ? m_folder : name;
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

void LibraryController::playIndex(int index)
{
    if (index < 0 || index >= m_currentFiles.size() || !m_player)
        return;

    m_player->setQueue(m_currentFiles, index, 0, true);
}

void LibraryController::playFromIndex(int index)
{
    playIndex(index);
}

void LibraryController::nextFolder()
{
    const QString next = m_folders.nextFolderPath(m_folder);
    if (!next.isEmpty())
        playFolder(next);
}

void LibraryController::previousFolder()
{
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

    const QString lastFolder = m_settings ? m_settings->lastFolder() : QString();
    if (lastFolder.isEmpty()) {
        m_restoreRequested = false;
        return;
    }
    m_scanner.scanPath(lastFolder);
}

void LibraryController::refresh()
{
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
