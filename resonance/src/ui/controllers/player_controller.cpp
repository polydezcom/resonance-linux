#include "ui/controllers/player_controller.h"

#include "core/audio/audio_engine.h"
#include "library/cover_art_cache.h"
#include "library/metadata_reader.h"
#include "ui/controllers/app_settings.h"
#include "ui/controllers/track_list_model.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QtConcurrent>

#include <algorithm>
#include <cmath>
#include <vector>

namespace resonance {
namespace {

RepeatMode engineRepeatMode(int mode)
{
    switch (mode) {
    case 1:
        return RepeatMode::Track;
    case 2:
        return RepeatMode::Folder;
    default:
        return RepeatMode::Off;
    }
}

} // namespace

PlayerController::PlayerController(AudioEngine *engine, AppSettings *settings,
                                   QObject *parent)
    : QObject(parent)
    , m_engine(engine)
    , m_settings(settings)
{
    m_volume = m_settings ? m_settings->volume() : 0.7;
    m_shuffle = m_settings ? m_settings->shuffle() : false;
    m_repeatMode = m_settings ? m_settings->repeatMode() : 0;

    m_engine->setVolume(static_cast<float>(m_volume));
    m_engine->setShuffle(m_shuffle);
    m_engine->setRepeatMode(engineRepeatMode(m_repeatMode));

    m_audioReady = m_engine->isInitialized();
    if (!m_audioReady) {
        m_error = QString::fromStdString(m_engine->takeError());
        if (m_error.isEmpty())
            m_error = tr("No audio device is available.");
    }

    connect(&m_coverWatcher, &QFutureWatcher<QString>::finished, this, [this] {
        const QString cachePath = m_coverWatcher.result();
        if (m_coverRequestPath != m_path)
            return;

        const QString url = cachePath.isEmpty()
            ? QString()
            : QUrl::fromLocalFile(cachePath).toString();
        if (url != m_coverUrl) {
            m_coverUrl = url;
            emit coverChanged();
        }
    });

    m_timer.setInterval(33);
    m_timer.setTimerType(Qt::CoarseTimer);
    connect(&m_timer, &QTimer::timeout, this, &PlayerController::poll);
    m_timer.start();

    m_stateTimer.setInterval(5000);
    m_stateTimer.setTimerType(Qt::VeryCoarseTimer);
    connect(&m_stateTimer, &QTimer::timeout, this,
            &PlayerController::savePlaybackState);
    m_stateTimer.start();

    if (QCoreApplication::instance()) {
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                this, &PlayerController::savePlaybackState);
    }

    updateStatus();
}

void PlayerController::setQueue(const QStringList &paths, int startIndex,
                                qint64 startPositionMs, bool autoPlay)
{
    m_queuePaths = paths;
    if (m_queuePaths.isEmpty()) {
        m_queueIndex = -1;
        m_hasTrack = false;
        m_playing = false;
        m_paused = false;
        m_finished = false;
        m_path.clear();
        m_title.clear();
        m_subtitle.clear();
        m_coverUrl.clear();
        m_positionMs = 0;
        m_durationMs = 0;
        m_engine->clearQueue();
        emit queueIndexChanged();
        emit trackChanged();
        emit playingChanged();
        emit pausedChanged();
        emit finishedChanged();
        emit coverChanged();
        emit positionChanged();
        emit durationChanged();
        updateStatus();
        return;
    }

    m_queueIndex = std::clamp(startIndex, 0, static_cast<int>(m_queuePaths.size()) - 1);

    std::vector<std::string> enginePaths;
    enginePaths.reserve(static_cast<std::size_t>(m_queuePaths.size()));
    for (const QString &path : m_queuePaths)
        enginePaths.push_back(path.toStdString());

    m_engine->setQueue(enginePaths, m_queueIndex,
                       static_cast<double>(std::max<qint64>(0, startPositionMs))
                           / 1000.0,
                       autoPlay);

    m_positionMs = std::max<qint64>(0, startPositionMs);
    m_durationMs = 0;
    m_finished = false;
    m_hasTrack = true;
    m_playing = false;
    m_paused = !autoPlay;

    emit queueIndexChanged();
    emit positionChanged();
    emit durationChanged();
    emit finishedChanged();
    emit playingChanged();
    emit pausedChanged();
    updateTrackInfo();
    updateStatus();
    savePlaybackState();
    poll();
}

void PlayerController::play(const QString &path, const QString &title)
{
    setQueue({path}, 0, 0, true);
    if (!title.trimmed().isEmpty() && !m_queuePaths.isEmpty()) {
        m_title = title;
        emit trackChanged();
    }
}

void PlayerController::playQueueIndex(int index)
{
    if (index < 0 || index >= m_queuePaths.size())
        return;

    m_queueIndex = index;
    m_engine->playQueueIndex(index);
    m_positionMs = 0;
    m_finished = false;
    emit positionChanged();
    emit finishedChanged();
    emit queueIndexChanged();
    updateTrackInfo();
    savePlaybackState();
    poll();
}

void PlayerController::playNext(const QString &path)
{
    if (path.isEmpty())
        return;

    if (m_queuePaths.isEmpty() || m_queueIndex < 0) {
        setQueue({path}, 0, 0, true);
        return;
    }

    const bool wasPlaying = m_playing;
    const qint64 position = m_positionMs;
    const int index = std::clamp(m_queueIndex, 0,
                                 static_cast<int>(m_queuePaths.size()) - 1);

    QStringList newQueue = m_queuePaths;
    newQueue.insert(index + 1, path);

    std::vector<std::string> enginePaths;
    enginePaths.reserve(static_cast<std::size_t>(newQueue.size()));
    for (const QString &item : newQueue)
        enginePaths.push_back(item.toStdString());

    m_queuePaths = newQueue;
    m_queueIndex = index;
    m_engine->setQueue(enginePaths, index,
                       static_cast<double>(std::max<qint64>(0, position)) / 1000.0,
                       wasPlaying);
    m_finished = false;

    emit queueIndexChanged();
    emit finishedChanged();
    updateTrackInfo();
    savePlaybackState();
    poll();
}

void PlayerController::next()
{
    m_engine->next();
    poll();
}

void PlayerController::previous()
{
    m_engine->previous();
    poll();
}

void PlayerController::togglePlayPause()
{
    if (!m_engine->hasTrack())
        return;
    m_engine->togglePause();
    poll();
}

void PlayerController::stop()
{
    m_engine->stop();
    m_positionMs = 0;
    emit positionChanged();
    savePlaybackState();
    poll();
}

void PlayerController::seek(qint64 positionMs)
{
    if (positionMs < 0)
        positionMs = 0;
    if (m_durationMs > 0)
        positionMs = std::min(positionMs, m_durationMs);

    m_engine->seekToSeconds(static_cast<double>(positionMs) / 1000.0);
    if (m_positionMs != positionMs) {
        m_positionMs = positionMs;
        emit positionChanged();
    }
    poll();
}

void PlayerController::setVolume(double volume)
{
    const double clamped = std::clamp(volume, 0.0, 1.0);
    if (std::abs(clamped - m_volume) < 1.0e-6)
        return;

    m_volume = clamped;
    m_engine->setVolume(static_cast<float>(m_volume));
    if (m_settings)
        m_settings->setVolume(m_volume);
    emit volumeChanged();
}

void PlayerController::setShuffle(bool shuffle)
{
    if (m_shuffle == shuffle)
        return;

    m_shuffle = shuffle;
    m_engine->setShuffle(shuffle);
    if (m_settings)
        m_settings->setShuffle(shuffle);
    emit shuffleChanged();
}

void PlayerController::toggleShuffle()
{
    setShuffle(!m_shuffle);
}

void PlayerController::setRepeatMode(int mode)
{
    const int clamped = std::clamp(mode, 0, 2);
    if (m_repeatMode == clamped)
        return;

    m_repeatMode = clamped;
    m_engine->setRepeatMode(engineRepeatMode(m_repeatMode));
    if (m_settings)
        m_settings->setRepeatMode(m_repeatMode);
    emit repeatModeChanged();
}

void PlayerController::cycleRepeat()
{
    setRepeatMode((m_repeatMode + 1) % 3);
}

void PlayerController::clearError()
{
    if (m_error.isEmpty())
        return;
    m_error.clear();
    emit errorChanged();
    updateStatus();
}

void PlayerController::refreshTrackInfo()
{
    updateTrackInfo();
}

void PlayerController::saveState()
{
    savePlaybackState();
}

QString PlayerController::formatTime(qint64 milliseconds) const
{
    const qint64 totalSeconds = std::max<qint64>(0, milliseconds) / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;

    if (hours > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }

    return QStringLiteral("%1:%2")
        .arg(minutes)
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

void PlayerController::poll()
{
    const bool ready = m_engine->isInitialized();
    if (ready != m_audioReady) {
        m_audioReady = ready;
        emit audioReadyChanged();
        updateStatus();
    }

    const int engineIndex = m_engine->currentIndex();
    if (engineIndex != m_queueIndex) {
        m_queueIndex = engineIndex;
        emit queueIndexChanged();
        updateTrackInfo();
    }

    const bool enginePlaying = m_engine->isPlaying();
    if (enginePlaying != m_playing) {
        m_playing = enginePlaying;
        emit playingChanged();
        updateStatus();
    }

    const bool enginePaused = m_engine->isPaused();
    if (enginePaused != m_paused) {
        m_paused = enginePaused;
        emit pausedChanged();
        updateStatus();
    }

    const bool engineFinished = m_engine->hasEnded();
    if (engineFinished != m_finished) {
        m_finished = engineFinished;
        emit finishedChanged();
        updateStatus();
    }

    const bool engineHasTrack = m_engine->hasTrack();
    if (engineHasTrack != m_hasTrack) {
        m_hasTrack = engineHasTrack;
        emit trackChanged();
        updateStatus();
    }

    const qint64 position = static_cast<qint64>(
        std::llround(m_engine->positionSeconds() * 1000.0));
    if (position != m_positionMs) {
        m_positionMs = position;
        emit positionChanged();
    }

    qint64 duration = static_cast<qint64>(
        std::llround(m_engine->durationSeconds() * 1000.0));
    if (duration <= 0 && m_trackModel) {
        const int row = m_trackModel->indexOfPath(m_path);
        if (row >= 0) {
            duration = m_trackModel->data(
                m_trackModel->index(row),
                TrackListModel::DurationRole).toLongLong();
        }
    }
    if (duration != m_durationMs) {
        m_durationMs = duration;
        emit durationChanged();
    }

    const std::string error = m_engine->takeError();
    if (!error.empty()) {
        m_error = QString::fromStdString(error);
        emit errorChanged();
        updateStatus();
    }
}

void PlayerController::updateTrackInfo()
{
    QString newPath;
    if (m_queueIndex >= 0 && m_queueIndex < m_queuePaths.size())
        newPath = m_queuePaths.at(m_queueIndex);
    else
        newPath = QString::fromStdString(m_engine->currentPath());

    QString title;
    QString subtitle;

    if (m_trackModel) {
        const int row = m_trackModel->indexOfPath(newPath);
        if (row >= 0) {
            const QModelIndex index = m_trackModel->index(row);
            title = m_trackModel->data(index, TrackListModel::TitleRole).toString();
            const QString artist =
                m_trackModel->data(index, TrackListModel::ArtistRole).toString();
            const QString album =
                m_trackModel->data(index, TrackListModel::AlbumRole).toString();
            if (!artist.isEmpty()) {
                subtitle = artist;
                if (!album.isEmpty())
                    subtitle += QStringLiteral(" · ") + album;
            }
        }
    }

    if (title.isEmpty() && !newPath.isEmpty())
        title = MetadataReader::fallbackTitle(newPath);
    if (subtitle.isEmpty() && !newPath.isEmpty()) {
        subtitle = QFileInfo(newPath).dir().dirName();
        if (subtitle.isEmpty())
            subtitle = QFileInfo(newPath).absolutePath();
    }

    const bool pathChanged = newPath != m_path;
    const bool changed = pathChanged || title != m_title || subtitle != m_subtitle;

    m_path = newPath;
    m_title = title;
    m_subtitle = subtitle;

    if (changed)
        emit trackChanged();

    if (pathChanged) {
        loadCover(newPath);
        savePlaybackState();
    }
}

void PlayerController::loadCover(const QString &path)
{
    m_coverRequestPath = path;

    if (m_coverUrl.isEmpty()) {
        // Nothing to clear.
    } else {
        m_coverUrl.clear();
        emit coverChanged();
    }

    if (path.isEmpty())
        return;

    m_coverWatcher.setFuture(QtConcurrent::run(
        [path] { return CoverArtCache::coverForTrack(path); }));
}

void PlayerController::savePlaybackState()
{
    if (!m_settings || m_path.isEmpty())
        return;

    m_settings->setLastTrack(m_path);
    m_settings->setLastPositionMs(m_positionMs);
}

void PlayerController::updateStatus()
{
    QString status;
    if (!m_audioReady) {
        status = m_error.isEmpty() ? tr("No audio device") : m_error;
    } else if (!m_error.isEmpty()) {
        status = m_error;
    } else if (!m_hasTrack) {
        status = tr("Open a folder to begin");
    } else if (m_finished) {
        status = tr("Finished");
    } else if (m_playing) {
        status = tr("Playing");
    } else if (m_paused) {
        status = tr("Paused");
    } else {
        status = tr("Stopped");
    }

    if (status != m_status) {
        m_status = status;
        emit statusChanged();
    }
}

} // namespace resonance
