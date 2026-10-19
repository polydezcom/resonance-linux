#pragma once

#include "library/folder_scanner.h"
#include "ui/controllers/track_list_model.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

namespace resonance {

class AppSettings;
class PlayerController;

class LibraryController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(resonance::TrackListModel *tracks READ tracks CONSTANT)
    Q_PROPERTY(QString folder READ folder NOTIFY folderChanged)
    Q_PROPERTY(QString folderName READ folderName NOTIFY folderChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(bool hasTracks READ hasTracks NOTIFY tracksChanged)
    Q_PROPERTY(int trackCount READ trackCount NOTIFY tracksChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString startupFolder READ startupFolder CONSTANT)
    Q_PROPERTY(bool startupAutoPlay READ startupAutoPlay CONSTANT)

public:
    LibraryController(PlayerController *player, AppSettings *settings,
                      QObject *parent = nullptr);

    TrackListModel *tracks() { return &m_tracks; }
    QString folder() const { return m_folder; }
    QString folderName() const;
    bool scanning() const { return m_scanner.isScanning(); }
    bool hasTracks() const { return m_tracks.count() > 0; }
    int trackCount() const { return m_tracks.count(); }
    QString error() const { return m_error; }
    QString startupFolder() const { return m_startupFolder; }
    bool startupAutoPlay() const { return m_startupAutoPlay; }

    void setStartupFolder(const QString &path, bool autoPlay);

    Q_INVOKABLE void openFolder(const QUrl &url);
    Q_INVOKABLE void openFolderPath(const QString &path, bool autoPlay = true);
    Q_INVOKABLE void openUrls(const QList<QUrl> &urls);
    Q_INVOKABLE void playIndex(int index);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void clearError();

signals:
    void folderChanged();
    void scanningChanged();
    void tracksChanged();
    void errorChanged();

private:
    void handleScanFinished(const QString &root, const QStringList &files);
    void handleScanFailed(const QString &message);
    void setFolder(const QString &folder);
    void setError(const QString &message);

    FolderScanner m_scanner;
    TrackListModel m_tracks;
    PlayerController *m_player = nullptr;
    AppSettings *m_settings = nullptr;
    QString m_folder;
    QString m_error;
    QString m_startupFolder;
    bool m_startupAutoPlay = false;
    bool m_autoPlay = true;
};

} // namespace resonance
