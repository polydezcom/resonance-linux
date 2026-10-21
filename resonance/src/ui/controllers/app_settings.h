#pragma once

#include <QObject>
#include <QString>

namespace resonance {

class AppSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark WRITE setDark NOTIFY darkChanged)
    Q_PROPERTY(QString accent READ accent WRITE setAccent NOTIFY accentChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QString lastFolder READ lastFolder WRITE setLastFolder NOTIFY lastFolderChanged)
    Q_PROPERTY(QString lastSelectedFolder READ lastSelectedFolder WRITE setLastSelectedFolder NOTIFY lastSelectedFolderChanged)
    Q_PROPERTY(QString lastTrack READ lastTrack WRITE setLastTrack NOTIFY lastTrackChanged)
    Q_PROPERTY(qint64 lastPositionMs READ lastPositionMs WRITE setLastPositionMs NOTIFY lastPositionChanged)
    Q_PROPERTY(bool shuffle READ shuffle WRITE setShuffle NOTIFY shuffleChanged)
    Q_PROPERTY(int repeatMode READ repeatMode WRITE setRepeatMode NOTIFY repeatModeChanged)
    Q_PROPERTY(QString lastMode READ lastMode WRITE setLastMode NOTIFY lastModeChanged)
    Q_PROPERTY(QString lastPlaylistId READ lastPlaylistId WRITE setLastPlaylistId NOTIFY lastPlaylistIdChanged)
    Q_PROPERTY(bool toolsOpen READ toolsOpen WRITE setToolsOpen NOTIFY toolsOpenChanged)

public:
    explicit AppSettings(QObject *parent = nullptr);

    bool dark() const { return m_dark; }
    void setDark(bool dark);

    QString accent() const { return m_accent; }
    void setAccent(const QString &accent);

    double volume() const { return m_volume; }
    void setVolume(double volume);

    QString lastFolder() const { return m_lastFolder; }
    void setLastFolder(const QString &folder);

    QString lastSelectedFolder() const { return m_lastSelectedFolder; }
    void setLastSelectedFolder(const QString &folder);

    QString lastTrack() const { return m_lastTrack; }
    void setLastTrack(const QString &path);

    qint64 lastPositionMs() const { return m_lastPositionMs; }
    void setLastPositionMs(qint64 positionMs);

    bool shuffle() const { return m_shuffle; }
    void setShuffle(bool shuffle);

    int repeatMode() const { return m_repeatMode; }
    void setRepeatMode(int mode);

    QString lastMode() const { return m_lastMode; }
    void setLastMode(const QString &mode);

    QString lastPlaylistId() const { return m_lastPlaylistId; }
    void setLastPlaylistId(const QString &id);

    bool toolsOpen() const { return m_toolsOpen; }
    void setToolsOpen(bool open);

signals:
    void darkChanged();
    void accentChanged();
    void volumeChanged();
    void lastFolderChanged();
    void lastSelectedFolderChanged();
    void lastTrackChanged();
    void lastPositionChanged();
    void shuffleChanged();
    void repeatModeChanged();
    void lastModeChanged();
    void lastPlaylistIdChanged();
    void toolsOpenChanged();

private:
    bool m_dark = true;
    QString m_accent = QStringLiteral("#7C9CFF");
    double m_volume = 0.7;
    QString m_lastFolder;
    QString m_lastSelectedFolder;
    QString m_lastTrack;
    qint64 m_lastPositionMs = 0;
    bool m_shuffle = false;
    int m_repeatMode = 0;
    QString m_lastMode = QStringLiteral("folder");
    QString m_lastPlaylistId;
    bool m_toolsOpen = false;
};

} // namespace resonance
