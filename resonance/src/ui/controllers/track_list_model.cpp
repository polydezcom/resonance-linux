#include "ui/controllers/track_list_model.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

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
    case PathRole:
        return entry.path;
    case FolderRole:
        return entry.folder;
    case DurationRole:
        return entry.durationMs;
    default:
        return {};
    }
}

QHash<int, QByteArray> TrackListModel::roleNames() const
{
    return {
        {TitleRole, QByteArrayLiteral("title")},
        {PathRole, QByteArrayLiteral("path")},
        {FolderRole, QByteArrayLiteral("folder")},
        {DurationRole, QByteArrayLiteral("durationMs")},
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
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).path == path)
            return i;
    }
    return -1;
}

void TrackListModel::setFiles(const QString &root, const QStringList &files)
{
    static const QRegularExpression leadingTrackNumber(
        QStringLiteral("^\\s*\\d{1,3}\\s*[-._)]\\s*"));

    QVector<Entry> entries;
    entries.reserve(files.size());

    const QDir rootDir(root);
    for (const QString &file : files) {
        const QFileInfo info(file);
        QString title = info.completeBaseName();
        title.remove(leadingTrackNumber);
        if (title.trimmed().isEmpty())
            title = info.completeBaseName();
        if (title.isEmpty())
            title = info.fileName();

        QString folder = rootDir.relativeFilePath(info.absolutePath());
        if (folder == QStringLiteral("."))
            folder.clear();

        entries.append({file, title, folder, 0});
    }

    beginResetModel();
    m_root = root;
    m_entries = std::move(entries);
    endResetModel();
    emit countChanged();
}

void TrackListModel::clear()
{
    if (m_entries.isEmpty())
        return;

    beginResetModel();
    m_entries.clear();
    m_root.clear();
    endResetModel();
    emit countChanged();
}

} // namespace resonance
