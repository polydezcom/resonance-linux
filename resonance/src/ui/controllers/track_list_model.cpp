#include "ui/controllers/track_list_model.h"

#include "library/metadata_reader.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <climits>

namespace resonance {

TrackListModel::TrackListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TrackListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_entries.size());
}

QVariant TrackListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
        || index.row() >= m_entries.size()) {
        return {};
    }

    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
        return entry.title;
    case ArtistRole:
        return entry.artist;
    case AlbumRole:
        return entry.album;
    case FolderRole:
        return entry.folder;
    case PathRole:
        return entry.path;
    case DurationRole:
        return entry.durationMs;
    case TrackNumberRole:
        return entry.trackNumber;
    case DiscNumberRole:
        return entry.discNumber;
    default:
        return {};
    }
}

QHash<int, QByteArray> TrackListModel::roleNames() const
{
    return {
        {TitleRole, QByteArrayLiteral("title")},
        {ArtistRole, QByteArrayLiteral("artist")},
        {AlbumRole, QByteArrayLiteral("album")},
        {FolderRole, QByteArrayLiteral("folder")},
        {PathRole, QByteArrayLiteral("path")},
        {DurationRole, QByteArrayLiteral("durationMs")},
        {TrackNumberRole, QByteArrayLiteral("trackNumber")},
        {DiscNumberRole, QByteArrayLiteral("discNumber")},
    };
}

QString TrackListModel::pathAt(int row) const
{
    if (row < 0 || row >= m_entries.size())
        return {};
    return m_entries.at(row).path;
}

QString TrackListModel::titleAt(int row) const
{
    if (row < 0 || row >= m_entries.size())
        return {};
    return m_entries.at(row).title;
}

int TrackListModel::indexOfPath(const QString &path) const
{
    return m_rowByPath.value(path, -1);
}

void TrackListModel::setFiles(const QString &root, const QStringList &files)
{
    QVector<Entry> entries;
    entries.reserve(files.size());
    QHash<QString, int> rowByPath;
    rowByPath.reserve(files.size());

    const QDir rootDir(root);
    for (const QString &file : files) {
        const QFileInfo info(file);
        QString folder = rootDir.relativeFilePath(info.absolutePath());
        if (folder == QStringLiteral("."))
            folder.clear();

        Entry entry;
        entry.path = file;
        entry.title = MetadataReader::fallbackTitle(file);
        entry.folder = folder;
        rowByPath.insert(file, entries.size());
        entries.append(entry);
    }

    beginResetModel();
    m_root = root;
    m_entries = std::move(entries);
    m_rowByPath = std::move(rowByPath);
    endResetModel();
    emit countChanged();
}

void TrackListModel::applyMetadata(const QVector<TrackMetadata> &metadata)
{
    if (metadata.isEmpty() || m_entries.isEmpty())
        return;

    static const QVector<int> roles = {
        TitleRole, ArtistRole, AlbumRole, DurationRole,
        TrackNumberRole, DiscNumberRole,
    };

    int firstRow = INT_MAX;
    int lastRow = -1;

    for (const TrackMetadata &item : metadata) {
        const auto found = m_rowByPath.constFind(item.path);
        if (found == m_rowByPath.constEnd())
            continue;

        const int row = found.value();
        Entry &entry = m_entries[row];
        if (!item.title.isEmpty())
            entry.title = item.title;
        entry.artist = item.artist;
        entry.album = item.album;
        entry.durationMs = item.durationMs;
        entry.trackNumber = item.trackNumber;
        entry.discNumber = item.discNumber;
        firstRow = std::min(firstRow, row);
        lastRow = std::max(lastRow, row);
    }

    if (lastRow >= firstRow) {
        emit dataChanged(index(firstRow), index(lastRow), roles);
    }
}

void TrackListModel::clear()
{
    if (m_entries.isEmpty())
        return;

    beginResetModel();
    m_entries.clear();
    m_rowByPath.clear();
    m_root.clear();
    endResetModel();
    emit countChanged();
}

} // namespace resonance
