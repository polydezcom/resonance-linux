#include "ui/controllers/playlist_controller.h"

#include "library/audio_file_utils.h"
#include "library/natural_sort.h"
#include "library/playlist_io.h"

#include <QDir>
#include <QFileInfo>
#include <QtConcurrent>

#include <algorithm>

namespace resonance {

PlaylistController::PlaylistController(QObject *parent)
    : QObject(parent)
{
    QString error;
    if (!m_manager.load(&error))
        setError(error);
    refreshModel();

    connect(&m_addWatcher, &QFutureWatcher<QStringList>::finished, this, [this] {
        const QString playlistId = m_pendingPlaylistId;
        m_pendingPlaylistId.clear();
        if (playlistId.isEmpty())
            return;

        const QStringList files = m_addWatcher.result();
        if (files.isEmpty()) {
            setError(tr("No supported audio files were found."));
            return;
        }

        if (!m_manager.addTracks(playlistId, files)) {
            setError(tr("The playlist no longer exists."));
            return;
        }
        if (!save())
            return;
        refreshModel();
        emit playlistTracksChanged(playlistId);
    });
}

QString PlaylistController::currentName() const
{
    return m_manager.name(m_currentId);
}

QStringList PlaylistController::tracks(const QString &id) const
{
    return m_manager.tracks(id);
}

QString PlaylistController::name(const QString &id) const
{
    return m_manager.name(id);
}

int PlaylistController::trackCount(const QString &id) const
{
    return static_cast<int>(m_manager.tracks(id).size());
}

QString PlaylistController::createPlaylist(const QString &name)
{
    const QString id = m_manager.create(name);
    if (!save())
        return {};

    m_currentId = id;
    m_model.setCurrentId(id);
    refreshModel();
    emit playlistAdded(id);
    emit currentIdChanged();
    return id;
}

void PlaylistController::renamePlaylist(const QString &id, const QString &name)
{
    if (!m_manager.rename(id, name)) {
        setError(tr("The playlist no longer exists."));
        return;
    }
    if (!save())
        return;
    refreshModel();
    emit playlistRenamed(id);
    emit currentIdChanged();
}

void PlaylistController::deletePlaylist(const QString &id)
{
    if (!m_manager.remove(id)) {
        setError(tr("The playlist no longer exists."));
        return;
    }
    if (!save())
        return;

    if (m_currentId == id)
        m_currentId.clear();
    m_model.setCurrentId(m_currentId);
    refreshModel();
    emit playlistRemoved(id);
    emit currentIdChanged();
}

void PlaylistController::setCurrent(const QString &id)
{
    if (!id.isEmpty() && !m_manager.playlist(id))
        return;
    if (m_currentId == id)
        return;

    m_currentId = id;
    m_model.setCurrentId(id);
    emit currentIdChanged();
}

void PlaylistController::addUrls(const QString &id, const QList<QUrl> &urls)
{
    if (!m_manager.playlist(id)) {
        setError(tr("The playlist no longer exists."));
        return;
    }

    QStringList localPaths;
    for (const QUrl &url : urls) {
        if (url.isLocalFile())
            localPaths.append(url.toLocalFile());
    }
    if (localPaths.isEmpty()) {
        setError(tr("Only local files and folders can be added."));
        return;
    }

    m_pendingPlaylistId = id;
    m_addWatcher.setFuture(QtConcurrent::run([localPaths] {
        QStringList files;
        for (const QString &path : localPaths)
            files.append(collectAudioFiles(path));
        std::sort(files.begin(), files.end(),
                  [](const QString &left, const QString &right) {
                      return naturalLess(left, right);
                  });
        return files;
    }));
}

void PlaylistController::addTracks(const QString &id, const QStringList &tracks)
{
    if (tracks.isEmpty())
        return;
    if (!m_manager.addTracks(id, tracks)) {
        setError(tr("The playlist no longer exists."));
        return;
    }
    if (!save())
        return;
    refreshModel();
    emit playlistTracksChanged(id);
}

void PlaylistController::removeTrack(const QString &id, int index)
{
    if (!m_manager.removeTrack(id, index))
        return;
    if (!save())
        return;
    refreshModel();
    emit playlistTracksChanged(id);
}

void PlaylistController::moveTrack(const QString &id, int from, int to)
{
    if (!m_manager.moveTrack(id, from, to))
        return;
    if (!save())
        return;
    refreshModel();
    emit playlistTracksChanged(id);
}

void PlaylistController::importPlaylist(const QUrl &fileUrl)
{
    if (!fileUrl.isLocalFile()) {
        setError(tr("Only local playlist files can be imported."));
        return;
    }

    const QString filePath = fileUrl.toLocalFile();
    QString error;
    const QStringList tracks = PlaylistIO::read(filePath, &error);
    if (!error.isEmpty()) {
        setError(error);
        return;
    }
    if (tracks.isEmpty()) {
        setError(tr("The playlist contains no tracks."));
        return;
    }

    const QString id = m_manager.create(QFileInfo(filePath).completeBaseName());
    m_manager.addTracks(id, tracks);
    if (!save())
        return;

    m_currentId = id;
    m_model.setCurrentId(id);
    refreshModel();
    emit playlistAdded(id);
    emit playlistTracksChanged(id);
    emit currentIdChanged();
}

void PlaylistController::exportPlaylist(const QString &id, const QUrl &fileUrl)
{
    if (!fileUrl.isLocalFile()) {
        setError(tr("Only local playlist files can be exported."));
        return;
    }
    if (!m_manager.playlist(id)) {
        setError(tr("The playlist no longer exists."));
        return;
    }

    QString error;
    if (!PlaylistIO::write(fileUrl.toLocalFile(), m_manager.tracks(id), &error))
        setError(error);
}

void PlaylistController::clearError()
{
    setError({});
}

bool PlaylistController::save()
{
    QString error;
    if (!m_manager.save(&error)) {
        setError(error);
        return false;
    }
    return true;
}

void PlaylistController::refreshModel()
{
    m_model.setPlaylists(m_manager.playlists());
    m_model.setCurrentId(m_currentId);
}

void PlaylistController::setError(const QString &message)
{
    if (m_error == message)
        return;
    m_error = message;
    emit errorChanged();
}

} // namespace resonance
