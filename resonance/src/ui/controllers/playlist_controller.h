#pragma once

#include "library/playlist_manager.h"
#include "ui/controllers/playlist_list_model.h"

#include <QFutureWatcher>
#include <QObject>
#include <QStringList>
#include <QUrl>

namespace resonance {

class PlaylistController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(resonance::PlaylistListModel *model READ model CONSTANT)
    Q_PROPERTY(QString currentId READ currentId NOTIFY currentIdChanged)
    Q_PROPERTY(QString currentName READ currentName NOTIFY currentIdChanged)
    Q_PROPERTY(bool hasCurrent READ hasCurrent NOTIFY currentIdChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit PlaylistController(QObject *parent = nullptr);

    PlaylistListModel *model() { return &m_model; }
    QString currentId() const { return m_currentId; }
    QString currentName() const;
    bool hasCurrent() const { return !m_currentId.isEmpty(); }
    QString error() const { return m_error; }

    const PlaylistManager &manager() const { return m_manager; }
    QStringList tracks(const QString &id) const;
    QString name(const QString &id) const;
    int trackCount(const QString &id) const;

    Q_INVOKABLE QString createPlaylist(const QString &name);
    Q_INVOKABLE void renamePlaylist(const QString &id, const QString &name);
    Q_INVOKABLE void deletePlaylist(const QString &id);
    Q_INVOKABLE void setCurrent(const QString &id);

    Q_INVOKABLE void addUrls(const QString &id, const QList<QUrl> &urls);
    Q_INVOKABLE void addTracks(const QString &id, const QStringList &tracks);
    Q_INVOKABLE void removeTrack(const QString &id, int index);
    Q_INVOKABLE void moveTrack(const QString &id, int from, int to);

    Q_INVOKABLE void importPlaylist(const QUrl &fileUrl);
    Q_INVOKABLE void exportPlaylist(const QString &id, const QUrl &fileUrl);
    Q_INVOKABLE void clearError();

signals:
    void currentIdChanged();
    void errorChanged();
    void playlistAdded(const QString &id);
    void playlistRemoved(const QString &id);
    void playlistRenamed(const QString &id);
    void playlistTracksChanged(const QString &id);

private:
    bool save();
    void refreshModel();
    void setError(const QString &message);

    PlaylistManager m_manager;
    PlaylistListModel m_model;
    QFutureWatcher<QStringList> m_addWatcher;
    QString m_pendingPlaylistId;
    QString m_currentId;
    QString m_error;
};

} // namespace resonance
