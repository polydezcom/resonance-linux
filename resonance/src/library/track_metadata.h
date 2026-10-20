#pragma once

#include <QByteArray>
#include <QList>
#include <QMetaType>
#include <QString>

namespace resonance {

struct TrackMetadata
{
    QString path;
    QString title;
    QString artist;
    QString album;
    int trackNumber = 0;
    int discNumber = 0;
    int year = 0;
    qint64 durationMs = 0;

    QByteArray coverData;
    QString coverMimeType;

    bool hasCover() const { return !coverData.isEmpty(); }
};

} // namespace resonance

Q_DECLARE_METATYPE(resonance::TrackMetadata)
Q_DECLARE_METATYPE(QList<resonance::TrackMetadata>)
