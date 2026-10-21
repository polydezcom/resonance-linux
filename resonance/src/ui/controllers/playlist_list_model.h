#pragma once

#include "library/playlist_manager.h"

#include <QAbstractListModel>
#include <QVector>

#include <QtQml/qqmlregistration.h>

namespace resonance {

class PlaylistListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Access playlists through Playlists.model")
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString currentId READ currentId NOTIFY currentIdChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TrackCountRole,
        CurrentRole,
    };
    Q_ENUM(Roles)

    explicit PlaylistListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return static_cast<int>(m_playlists.size()); }
    QString currentId() const { return m_currentId; }
    Q_INVOKABLE QString idAt(int row) const;
    Q_INVOKABLE int rowForId(const QString &id) const;

    void setPlaylists(const QVector<Playlist> &playlists);
    void setCurrentId(const QString &id);

signals:
    void countChanged();
    void currentIdChanged();

private:
    QVector<Playlist> m_playlists;
    QString m_currentId;
};

} // namespace resonance
