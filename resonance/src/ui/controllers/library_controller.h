#pragma once

#include "library/folder_scanner.h"
#include "library/metadata_loader.h"
#include "library/track_metadata.h"
#include "ui/controllers/folder_tree_model.h"
#include "ui/controllers/track_list_model.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>

namespace resonance {

class AppSettings;
class PlayerController;

class LibraryController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(resonance::TrackListModel *tracks READ tracks CONSTANT)
    Q_PROPERTY(resonance::FolderTreeModel *folders READ folders CONSTANT)
    Q_PROPERTY(QString folder READ folder NOTIFY folderChanged)
    Q_PROPERTY(QString rootFolder READ rootFolder NOTIFY folderChanged)
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
    FolderTreeModel *folders() { return &m_folders; }
    QString folder() const { return m_folder; }
    QString rootFolder() const { return m_rootFolder; }
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
    Q_INVOKABLE void playFolder(const QString &folder);
    Q_INVOKABLE void playIndex(int index);
    Q_INVOKABLE void playFromIndex(int index);
    Q_INVOKABLE void nextFolder();
    Q_INVOKABLE void previousFolder();
    Q_INVOKABLE void restoreLastSession();
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
    void setSelectedFolder(const QString &folder, bool autoPlay);
    void setError(const QString &message);
    void applyCachedMetadata();
    void startMetadataLoad(const QStringList &files);
    QStringList filesUnder(const QString &folder) const;

    FolderScanner m_scanner;
    MetadataLoader m_metadataLoader;
    TrackListModel m_tracks;
    FolderTreeModel m_folders;
    PlayerController *m_player = nullptr;
    AppSettings *m_settings = nullptr;

    QStringList m_allFiles;
    QStringList m_currentFiles;
    QHash<QString, TrackMetadata> m_metadataCache;

    QString m_rootFolder;
    QString m_folder;
    QString m_error;
    QString m_startupFolder;
    bool m_startupAutoPlay = false;

    bool m_openAutoPlay = true;
    bool m_restoreRequested = false;
    bool m_refreshRequested = false;
    QString m_refreshTrack;
    QString m_refreshFolder;
    qint64 m_refreshPosition = 0;
    bool m_refreshAutoPlay = false;
};

} // namespace resonance
