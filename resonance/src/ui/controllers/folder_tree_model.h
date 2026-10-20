#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QStringList>
#include <QVector>

#include <QtQml/qqmlregistration.h>

namespace resonance {

class FolderTreeModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Access the folder tree through Library.folders")
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString currentFolder READ currentFolder NOTIFY currentFolderChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        DepthRole,
        HasChildrenRole,
        ExpandedRole,
        TrackCountRole,
        CurrentRole,
        RootRole,
    };
    Q_ENUM(Roles)

    explicit FolderTreeModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return static_cast<int>(m_visible.size()); }
    QString currentFolder() const { return m_currentFolder; }
    Q_INVOKABLE QString pathAt(int row) const;

    void setRoot(const QString &root, const QStringList &files);
    void clear();

    Q_INVOKABLE void toggle(int row);
    Q_INVOKABLE void setCurrentFolder(const QString &path);
    Q_INVOKABLE int rowForPath(const QString &path) const;
    Q_INVOKABLE QString nextFolderPath(const QString &path) const;
    Q_INVOKABLE QString previousFolderPath(const QString &path) const;

signals:
    void countChanged();
    void currentFolderChanged();

private:
    struct Node
    {
        QString path;
        QString name;
        int parent = -1;
        int depth = 0;
        bool expanded = false;
        int totalTracks = 0;
        QVector<int> children;
    };

    int addDirectory(const QString &path);
    void rebuildVisible();
    void rebuildDfsOrder();
    int nodeIndexForPath(const QString &path) const;

    QVector<Node> m_nodes;
    QVector<int> m_visible;
    QVector<int> m_dfsOrder;
    QHash<QString, int> m_indexByPath;
    QString m_root;
    QString m_currentFolder;
};

} // namespace resonance
