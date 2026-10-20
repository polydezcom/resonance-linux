#pragma once

#include "library/track_metadata.h"

#include <QAbstractListModel>
#include <QHash>
#include <QStringList>
#include <QVector>

#include <QtQml/qqmlregistration.h>

namespace resonance {

class TrackListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Access the track list through Library.tracks")
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        ArtistRole,
        AlbumRole,
        FolderRole,
        PathRole,
        DurationRole,
        TrackNumberRole,
        DiscNumberRole,
    };
    Q_ENUM(Roles)

    explicit TrackListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return static_cast<int>(m_entries.size()); }
    QString pathAt(int row) const;
    QString titleAt(int row) const;
    Q_INVOKABLE int indexOfPath(const QString &path) const;

    void setFiles(const QString &root, const QStringList &files);
    void applyMetadata(const QVector<TrackMetadata> &metadata);
    void clear();

signals:
    void countChanged();

private:
    struct Entry
    {
        QString path;
        QString title;
        QString artist;
        QString album;
        QString folder;
        qint64 durationMs = 0;
        int trackNumber = 0;
        int discNumber = 0;
    };

    QVector<Entry> m_entries;
    QHash<QString, int> m_rowByPath;
    QString m_root;
};

} // namespace resonance
