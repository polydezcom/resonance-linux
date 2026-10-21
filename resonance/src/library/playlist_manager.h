#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace resonance {

struct Playlist
{
    QString id;
    QString name;
    QStringList tracks;
};

/**
 * Playlist collection with atomic JSON persistence. The manager is pure data
 * logic; the UI controller calls save() after mutations so write errors can be
 * surfaced.
 */
class PlaylistManager
{
public:
    explicit PlaylistManager(QString storagePath = {});

    static QString defaultStoragePath();

    bool load(QString *error = nullptr);
    bool save(QString *error = nullptr);

    const QVector<Playlist> &playlists() const { return m_playlists; }
    int indexOfId(const QString &id) const;
    const Playlist *playlist(const QString &id) const;

    QString create(const QString &name);
    bool remove(const QString &id);
    bool rename(const QString &id, const QString &name);
    bool setTracks(const QString &id, const QStringList &tracks);
    bool addTracks(const QString &id, const QStringList &tracks);
    bool removeTrack(const QString &id, int index);
    bool moveTrack(const QString &id, int from, int to);

    QStringList tracks(const QString &id) const;
    QString name(const QString &id) const;
    QString storagePath() const { return m_storagePath; }

    static QString cleanName(const QString &name);

private:
    QString uniqueName(const QString &base) const;

    QString m_storagePath;
    QVector<Playlist> m_playlists;
};

} // namespace resonance
