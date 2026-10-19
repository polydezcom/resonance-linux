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

    bool toolsOpen() const { return m_toolsOpen; }
    void setToolsOpen(bool open);

signals:
    void darkChanged();
    void accentChanged();
    void volumeChanged();
    void lastFolderChanged();
    void toolsOpenChanged();

private:
    bool m_dark = true;
    QString m_accent = QStringLiteral("#7C9CFF");
    double m_volume = 0.7;
    QString m_lastFolder;
    bool m_toolsOpen = false;
};

} // namespace resonance
