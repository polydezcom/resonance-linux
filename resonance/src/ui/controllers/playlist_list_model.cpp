#include "ui/controllers/playlist_list_model.h"

namespace resonance {

PlaylistListModel::PlaylistListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PlaylistListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_playlists.size());
}

QVariant PlaylistListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
        || index.row() >= m_playlists.size()) {
        return {};
    }

    const Playlist &playlist = m_playlists.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return playlist.name;
    case IdRole:
        return playlist.id;
    case TrackCountRole:
        return static_cast<int>(playlist.tracks.size());
    case CurrentRole:
        return playlist.id == m_currentId;
    default:
        return {};
    }
}

QHash<int, QByteArray> PlaylistListModel::roleNames() const
{
    return {
        {IdRole, QByteArrayLiteral("playlistId")},
        {NameRole, QByteArrayLiteral("name")},
        {TrackCountRole, QByteArrayLiteral("trackCount")},
        {CurrentRole, QByteArrayLiteral("current")},
    };
}

QString PlaylistListModel::idAt(int row) const
{
    if (row < 0 || row >= m_playlists.size())
        return {};
    return m_playlists.at(row).id;
}

int PlaylistListModel::rowForId(const QString &id) const
{
    for (int i = 0; i < m_playlists.size(); ++i) {
        if (m_playlists.at(i).id == id)
            return i;
    }
    return -1;
}

void PlaylistListModel::setPlaylists(const QVector<Playlist> &playlists)
{
    beginResetModel();
    m_playlists = playlists;
    endResetModel();
    emit countChanged();
}

void PlaylistListModel::setCurrentId(const QString &id)
{
    if (m_currentId == id)
        return;

    const int oldRow = rowForId(m_currentId);
    m_currentId = id;
    const int newRow = rowForId(m_currentId);

    if (oldRow >= 0)
        emit dataChanged(index(oldRow), index(oldRow), {CurrentRole});
    if (newRow >= 0)
        emit dataChanged(index(newRow), index(newRow), {CurrentRole});
    emit currentIdChanged();
}

} // namespace resonance
