#include "ui/controllers/folder_tree_model.h"

#include "library/natural_sort.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <functional>

namespace resonance {

FolderTreeModel::FolderTreeModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int FolderTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_visible.size());
}

QVariant FolderTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0
        || index.row() >= m_visible.size()) {
        return {};
    }

    const Node &node = m_nodes.at(m_visible.at(index.row()));
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return node.name;
    case PathRole:
        return node.path;
    case DepthRole:
        return node.depth;
    case HasChildrenRole:
        return !node.children.isEmpty();
    case ExpandedRole:
        return node.expanded;
    case TrackCountRole:
        return node.totalTracks;
    case CurrentRole:
        return node.path == m_currentFolder;
    case RootRole:
        return node.path == m_root;
    default:
        return {};
    }
}

QHash<int, QByteArray> FolderTreeModel::roleNames() const
{
    return {
        {NameRole, QByteArrayLiteral("name")},
        {PathRole, QByteArrayLiteral("path")},
        {DepthRole, QByteArrayLiteral("depth")},
        {HasChildrenRole, QByteArrayLiteral("hasChildren")},
        {ExpandedRole, QByteArrayLiteral("expanded")},
        {TrackCountRole, QByteArrayLiteral("trackCount")},
        {CurrentRole, QByteArrayLiteral("current")},
        {RootRole, QByteArrayLiteral("root")},
    };
}

QString FolderTreeModel::pathAt(int row) const
{
    if (row < 0 || row >= m_visible.size())
        return {};
    return m_nodes.at(m_visible.at(row)).path;
}

void FolderTreeModel::setRoot(const QString &root, const QStringList &files)
{
    beginResetModel();

    m_nodes.clear();
    m_visible.clear();
    m_dfsOrder.clear();
    m_indexByPath.clear();
    m_root = root;
    m_currentFolder.clear();

    if (root.isEmpty()) {
        endResetModel();
        emit countChanged();
        emit currentFolderChanged();
        return;
    }

    Node rootNode;
    rootNode.path = root;
    rootNode.name = QFileInfo(root).fileName();
    if (rootNode.name.isEmpty())
        rootNode.name = root;
    rootNode.expanded = true;
    m_nodes.append(rootNode);
    m_indexByPath.insert(root, 0);

    for (const QString &file : files) {
        const QString directory = QFileInfo(file).absolutePath();
        const int dirIndex = addDirectory(directory);
        if (dirIndex < 0)
            continue;

        for (int i = dirIndex; i >= 0; i = m_nodes[i].parent)
            ++m_nodes[i].totalTracks;
    }

    std::function<void(int)> sortChildren = [&](int index) {
        Node &node = m_nodes[index];
        std::sort(node.children.begin(), node.children.end(),
                  [this](int left, int right) {
                      return naturalLess(m_nodes[left].name, m_nodes[right].name);
                  });
        for (int child : node.children)
            sortChildren(child);
    };
    sortChildren(0);

    m_currentFolder = root;
    rebuildDfsOrder();
    rebuildVisible();

    endResetModel();
    emit countChanged();
    emit currentFolderChanged();
}

void FolderTreeModel::clear()
{
    beginResetModel();
    m_nodes.clear();
    m_visible.clear();
    m_dfsOrder.clear();
    m_indexByPath.clear();
    m_root.clear();
    m_currentFolder.clear();
    endResetModel();
    emit countChanged();
    emit currentFolderChanged();
}

void FolderTreeModel::toggle(int row)
{
    if (row < 0 || row >= m_visible.size())
        return;

    const int nodeIndex = m_visible.at(row);
    Node &node = m_nodes[nodeIndex];
    if (node.children.isEmpty())
        return;

    node.expanded = !node.expanded;
    beginResetModel();
    rebuildVisible();
    endResetModel();
    emit countChanged();
}

void FolderTreeModel::setCurrentFolder(const QString &path)
{
    const int index = nodeIndexForPath(path);
    if (index < 0)
        return;

    for (int i = index; i >= 0; i = m_nodes[i].parent)
        m_nodes[i].expanded = true;

    m_currentFolder = m_nodes[index].path;
    beginResetModel();
    rebuildVisible();
    endResetModel();
    emit countChanged();
    emit currentFolderChanged();
}

int FolderTreeModel::rowForPath(const QString &path) const
{
    const int nodeIndex = nodeIndexForPath(path);
    if (nodeIndex < 0)
        return -1;
    return m_visible.indexOf(nodeIndex);
}

QString FolderTreeModel::nextFolderPath(const QString &path) const
{
    const int nodeIndex = nodeIndexForPath(path);
    if (nodeIndex < 0)
        return {};
    const int position = m_dfsOrder.indexOf(nodeIndex);
    if (position < 0 || position + 1 >= m_dfsOrder.size())
        return {};
    return m_nodes.at(m_dfsOrder.at(position + 1)).path;
}

QString FolderTreeModel::previousFolderPath(const QString &path) const
{
    const int nodeIndex = nodeIndexForPath(path);
    if (nodeIndex < 0)
        return {};
    const int position = m_dfsOrder.indexOf(nodeIndex);
    if (position <= 0)
        return {};
    return m_nodes.at(m_dfsOrder.at(position - 1)).path;
}

int FolderTreeModel::addDirectory(const QString &path)
{
    if (m_nodes.isEmpty())
        return -1;

    const auto existing = m_indexByPath.constFind(path);
    if (existing != m_indexByPath.constEnd())
        return existing.value();

    const QString relative = QDir(m_root).relativeFilePath(path);
    if (relative.isEmpty() || relative == QStringLiteral("."))
        return 0;

    const QStringList parts = relative.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    int parent = 0;
    QString current = m_root;

    for (const QString &part : parts) {
        current = QDir(current).filePath(part);
        const auto found = m_indexByPath.constFind(current);
        if (found != m_indexByPath.constEnd()) {
            parent = found.value();
            continue;
        }

        Node node;
        node.path = current;
        node.name = part;
        node.parent = parent;
        node.depth = m_nodes[parent].depth + 1;
        const int index = m_nodes.size();
        m_nodes.append(node);
        m_nodes[parent].children.append(index);
        m_indexByPath.insert(current, index);
        parent = index;
    }

    return parent;
}

void FolderTreeModel::rebuildVisible()
{
    m_visible.clear();
    if (m_nodes.isEmpty())
        return;

    std::function<void(int)> appendNode = [&](int index) {
        m_visible.append(index);
        const Node &node = m_nodes[index];
        if (!node.expanded)
            return;
        for (int child : node.children)
            appendNode(child);
    };

    appendNode(0);
}

void FolderTreeModel::rebuildDfsOrder()
{
    m_dfsOrder.clear();
    if (m_nodes.isEmpty())
        return;

    std::function<void(int)> appendNode = [&](int index) {
        m_dfsOrder.append(index);
        for (int child : m_nodes[index].children)
            appendNode(child);
    };
    appendNode(0);
}

int FolderTreeModel::nodeIndexForPath(const QString &path) const
{
    return m_indexByPath.value(path, -1);
}

} // namespace resonance
