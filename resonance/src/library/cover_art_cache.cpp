#include "library/cover_art_cache.h"

#include "library/metadata_reader.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QStandardPaths>

namespace resonance {
namespace {

QString cacheDirectory()
{
    const QString path = QStandardPaths::writableLocation(
        QStandardPaths::CacheLocation) + QStringLiteral("/covers");
    QDir().mkpath(path);
    return path;
}

QString cacheKey(const QString &trackPath)
{
    const QFileInfo info(trackPath);
    const QFileInfo folderInfo(info.absolutePath());
    const QByteArray key = (info.absoluteFilePath()
                            + QLatin1Char('|')
                            + QString::number(info.size())
                            + QLatin1Char('|')
                            + QString::number(info.lastModified().toMSecsSinceEpoch())
                            + QLatin1Char('|')
                            + QString::number(folderInfo.lastModified().toMSecsSinceEpoch()))
                               .toUtf8();
    return QString::fromLatin1(
        QCryptographicHash::hash(key, QCryptographicHash::Sha1).toHex());
}

QString cachedFile(const QString &directory, const QString &key)
{
    const QString jpeg = directory + QLatin1Char('/') + key + QStringLiteral(".jpg");
    if (QFileInfo::exists(jpeg))
        return jpeg;
    const QString png = directory + QLatin1Char('/') + key + QStringLiteral(".png");
    if (QFileInfo::exists(png))
        return png;
    return {};
}

bool loadSidecar(const QString &trackPath, QImage *image)
{
    static const QStringList names = {
        QStringLiteral("cover.jpg"),
        QStringLiteral("cover.jpeg"),
        QStringLiteral("cover.png"),
        QStringLiteral("folder.jpg"),
        QStringLiteral("folder.jpeg"),
        QStringLiteral("folder.png"),
        QStringLiteral("front.jpg"),
        QStringLiteral("front.jpeg"),
        QStringLiteral("front.png"),
        QStringLiteral("album.jpg"),
        QStringLiteral("album.jpeg"),
        QStringLiteral("album.png"),
    };

    const QDir directory = QFileInfo(trackPath).dir();
    for (const QString &name : names) {
        const QString candidate = directory.filePath(name);
        if (!QFileInfo::exists(candidate))
            continue;
        QImageReader reader(candidate);
        reader.setAutoTransform(true);
        QImage loaded = reader.read();
        if (!loaded.isNull()) {
            *image = loaded;
            return true;
        }
    }
    return false;
}

} // namespace

QString CoverArtCache::coverForTrack(const QString &trackPath)
{
    if (trackPath.isEmpty())
        return {};

    const QString directory = cacheDirectory();
    const QString key = cacheKey(trackPath);
    if (!directory.isEmpty()) {
        const QString existing = cachedFile(directory, key);
        if (!existing.isEmpty())
            return existing;
    }

    QImage image;
    const TrackMetadata metadata = MetadataReader::read(trackPath, true);
    if (metadata.hasCover()) {
        if (!image.loadFromData(metadata.coverData,
                                metadata.coverMimeType.toUtf8().constData())
            && !image.loadFromData(metadata.coverData)) {
            image = QImage();
        }
    }

    if (image.isNull())
        loadSidecar(trackPath, &image);

    if (image.isNull())
        return {};

    constexpr int maxSize = 640;
    if (image.width() > maxSize || image.height() > maxSize) {
        image = image.scaled(maxSize, maxSize, Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    }

    const QString extension = image.hasAlphaChannel()
        ? QStringLiteral(".png") : QStringLiteral(".jpg");
    const QString cachePath = directory + QLatin1Char('/') + key + extension;
    if (!image.save(cachePath))
        return {};
    return cachePath;
}

} // namespace resonance
