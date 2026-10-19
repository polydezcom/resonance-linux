#include "ui/controllers/player_controller.h"

#include "core/audio/audio_engine.h"
#include "ui/controllers/app_settings.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <cmath>

namespace resonance {

PlayerController::PlayerController(AudioEngine *engine, AppSettings *settings,
                                   QObject *parent)
    : QObject(parent)
    , m_engine(engine)
    , m_settings(settings)
{
    m_volume = m_settings ? m_settings->volume() : 0.7;
    m_engine->setVolume(static_cast<float>(m_volume));

    m_audioReady = m_engine->isInitialized();
    if (!m_audioReady) {
        m_error = QString::fromStdString(m_engine->takeError());
        if (m_error.isEmpty())
            m_error = tr("No audio device is available.");
    }

    m_timer.setInterval(33);
    m_timer.setTimerType(Qt::CoarseTimer);
    connect(&m_timer, &QTimer::timeout, this, &PlayerController::poll);
    m_timer.start();

    updateStatus();
}

void PlayerController::play(const QString &path, const QString &title)
{
    if (path.isEmpty())
        return;

    const QFileInfo info(path);
    m_path = path;
    m_title = title.trimmed().isEmpty() ? info.completeBaseName() : title;
    if (m_title.isEmpty())
        m_title = info.fileName();
    m_subtitle = info.dir().dirName();
    if (m_subtitle.isEmpty())
        m_subtitle = info.absolutePath();

    m_positionMs = 0;
    m_durationMs = 0;
    m_finished = false;
    m_hasTrack = true;
    m_error.clear();

    emit trackChanged();
    emit positionChanged();
    emit durationChanged();
    emit finishedChanged();
    emit errorChanged();
    updateStatus();

    m_engine->playFile(path.toStdString());
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
    if (m_positionMs != 0) {
        m_positionMs = 0;
        emit positionChanged();
    }
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

void PlayerController::clearError()
{
    if (m_error.isEmpty())
        return;
    m_error.clear();
    emit errorChanged();
    updateStatus();
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

    const qint64 duration = static_cast<qint64>(
        std::llround(m_engine->durationSeconds() * 1000.0));
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
