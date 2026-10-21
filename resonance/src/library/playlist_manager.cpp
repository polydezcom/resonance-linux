#include "library/playlist_manager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QObject>
#include <QSaveFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUuid>

#include <algorithm>
#include <utility>

namespace resonance {
PlaylistManager::PlaylistManager(QString storagePath)
    : m_storagePath(storagePath.isEmpty() ? defaultStoragePath()
                                          : std::move(storagePath))
{
}

QString PlaylistManager::defaultStoragePath()
{
    const QString directory =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return directory + QStringLiteral("/playlists.json");
}

bool PlaylistManager::load(QString *error)
{
    m_playlists.clear();

    QFile file(m_storagePath);
    if (!file.exists())
        return true;
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QObject::tr("Could not open playlists: %1").arg(file.errorString());
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error)
            *error = QObject::tr("Playlist file is not valid JSON: %1")
                         .arg(parseError.errorString());
        return false;
    }
    if (!document.isObject()) {
        if (error)
            *error = QObject::tr("Playlist file has an unexpected structure.");
        return false;
    }

    const QJsonObject root = document.object();
    const QJsonArray playlists = root.value(QStringLiteral("playlists")).toArray();
    for (const QJsonValue &value : playlists) {
        if (!value.isObject())
            continue;
        const QJsonObject object = value.toObject();
        Playlist playlist;
        playlist.id = object.value(QStringLiteral("id")).toString();
        if (playlist.id.isEmpty())
            playlist.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (indexOfId(playlist.id) >= 0)
            continue;

        playlist.name = cleanName(object.value(QStringLiteral("name")).toString());
        const QJsonArray tracks = object.value(QStringLiteral("tracks")).toArray();
        for (const QJsonValue &track : tracks) {
            const QString path = track.toString();
            if (!path.isEmpty())
                playlist.tracks.append(path);
        }
        m_playlists.append(playlist);
    }

    return true;
}

bool PlaylistManager::save(QString *error)
{
    const QFileInfo info(m_storagePath);
    if (!QDir().mkpath(info.absolutePath())) {
        if (error)
            *error = QObject::tr("Could not create the playlist directory.");
        return false;
    }

    QJsonArray playlists;
    for (const Playlist &playlist : m_playlists) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), playlist.id);
        object.insert(QStringLiteral("name"), playlist.name);
        QJsonArray tracks;
        for (const QString &track : playlist.tracks)
            tracks.append(track);
        object.insert(QStringLiteral("tracks"), tracks);
        playlists.append(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("playlists"), playlists);

    QSaveFile file(m_storagePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error)
            *error = QObject::tr("Could not write playlists: %1").arg(file.errorString());
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (error)
            *error = QObject::tr("Could not save playlists: %1").arg(file.errorString());
        return false;
    }
    return true;
}

int PlaylistManager::indexOfId(const QString &id) const
{
    for (int i = 0; i < m_playlists.size(); ++i) {
        if (m_playlists.at(i).id == id)
            return i;
    }
    return -1;
}

const Playlist *PlaylistManager::playlist(const QString &id) const
{
    const int index = indexOfId(id);
    return index < 0 ? nullptr : &m_playlists.at(index);
}

QString PlaylistManager::create(const QString &name)
{
    Playlist playlist;
    playlist.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    playlist.name = uniqueName(cleanName(name));
    m_playlists.append(playlist);
    return playlist.id;
}

bool PlaylistManager::remove(const QString &id)
{
    const int index = indexOfId(id);
    if (index < 0)
        return false;
    m_playlists.removeAt(index);
    return true;
}

bool PlaylistManager::rename(const QString &id, const QString &name)
{
    const int index = indexOfId(id);
    if (index < 0)
        return false;
    const QString cleaned = cleanName(name);
    if (cleaned == m_playlists.at(index).name)
        return true;
    m_playlists[index].name = uniqueName(cleaned);
    return true;
}

bool PlaylistManager::setTracks(const QString &id, const QStringList &tracks)
{
    const int index = indexOfId(id);
    if (index < 0)
        return false;

    QStringList cleaned;
    cleaned.reserve(tracks.size());
    for (const QString &track : tracks) {
        if (!track.isEmpty())
            cleaned.append(track);
    }
    m_playlists[index].tracks = cleaned;
    return true;
}

bool PlaylistManager::addTracks(const QString &id, const QStringList &tracks)
{
    const int index = indexOfId(id);
    if (index < 0)
        return false;

    for (const QString &track : tracks) {
        if (!track.isEmpty())
            m_playlists[index].tracks.append(track);
    }
    return true;
}

bool PlaylistManager::removeTrack(const QString &id, int index)
{
    const int playlistIndex = indexOfId(id);
    if (playlistIndex < 0)
        return false;
    auto &tracks = m_playlists[playlistIndex].tracks;
    if (index < 0 || index >= tracks.size())
        return false;
    tracks.removeAt(index);
    return true;
}

bool PlaylistManager::moveTrack(const QString &id, int from, int to)
{
    const int playlistIndex = indexOfId(id);
    if (playlistIndex < 0)
        return false;

    auto &tracks = m_playlists[playlistIndex].tracks;
    if (from < 0 || from >= tracks.size() || to < 0 || to >= tracks.size()
        || from == to) {
        return false;
    }

    const QString track = tracks.takeAt(from);
    to = std::clamp(to, 0, static_cast<int>(tracks.size()));
    tracks.insert(to, track);
    return true;
}

QStringList PlaylistManager::tracks(const QString &id) const
{
    const Playlist *value = playlist(id);
    return value ? value->tracks : QStringList();
}

QString PlaylistManager::name(const QString &id) const
{
    const Playlist *value = playlist(id);
    return value ? value->name : QString();
}

QString PlaylistManager::cleanName(const QString &name)
{
    QString cleaned = name.simplified();
    cleaned.remove(QRegularExpression(QStringLiteral("[\\x00-\\x1f]")));
    if (cleaned.isEmpty())
        cleaned = QStringLiteral("Untitled playlist");
    return cleaned.left(120);
}

QString PlaylistManager::uniqueName(const QString &base) const
{
    QString candidate = base;
    int suffix = 2;
    while (std::any_of(m_playlists.cbegin(), m_playlists.cend(),
                       [&candidate](const Playlist &playlist) {
                           return playlist.name.compare(candidate,
                                                        Qt::CaseInsensitive) == 0;
                       })) {
        candidate = QStringLiteral("%1 (%2)").arg(base).arg(suffix++);
    }
    return candidate;
}

} // namespace resonance
