#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

namespace resonance {

class AppSettings;
class AudioEngine;

class PlayerController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(bool finished READ finished NOTIFY finishedChanged)
    Q_PROPERTY(bool hasTrack READ hasTrack NOTIFY trackChanged)
    Q_PROPERTY(bool audioReady READ audioReady NOTIFY audioReadyChanged)
    Q_PROPERTY(QString title READ title NOTIFY trackChanged)
    Q_PROPERTY(QString subtitle READ subtitle NOTIFY trackChanged)
    Q_PROPERTY(QString path READ path NOTIFY trackChanged)
    Q_PROPERTY(qint64 positionMs READ positionMs NOTIFY positionChanged)
    Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY durationChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    PlayerController(AudioEngine *engine, AppSettings *settings,
                     QObject *parent = nullptr);

    bool playing() const { return m_playing; }
    bool paused() const { return m_paused; }
    bool finished() const { return m_finished; }
    bool hasTrack() const { return m_hasTrack; }
    bool audioReady() const { return m_audioReady; }
    QString title() const { return m_title; }
    QString subtitle() const { return m_subtitle; }
    QString path() const { return m_path; }
    qint64 positionMs() const { return m_positionMs; }
    qint64 durationMs() const { return m_durationMs; }
    double volume() const { return m_volume; }
    QString status() const { return m_status; }
    QString error() const { return m_error; }

    Q_INVOKABLE void play(const QString &path, const QString &title);
    Q_INVOKABLE void togglePlayPause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seek(qint64 positionMs);
    Q_INVOKABLE void setVolume(double volume);
    Q_INVOKABLE void clearError();
    Q_INVOKABLE QString formatTime(qint64 milliseconds) const;

signals:
    void playingChanged();
    void pausedChanged();
    void finishedChanged();
    void trackChanged();
    void audioReadyChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void statusChanged();
    void errorChanged();

private:
    void poll();
    void updateStatus();

    AudioEngine *m_engine = nullptr;
    AppSettings *m_settings = nullptr;
    QTimer m_timer;

    bool m_playing = false;
    bool m_paused = false;
    bool m_finished = false;
    bool m_hasTrack = false;
    bool m_audioReady = false;
    QString m_title;
    QString m_subtitle;
    QString m_path;
    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;
    double m_volume = 0.7;
    QString m_status;
    QString m_error;
};

} // namespace resonance
