#include "library/metadata_reader.h"

#include <QFileInfo>
#include <QRegularExpression>

#ifdef RESONANCE_HAS_TAGLIB
#include <taglib/audioproperties.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tbytevector.h>
#include <taglib/tstring.h>
#include <taglib/tvariant.h>
#endif

namespace resonance {
namespace {

#ifdef RESONANCE_HAS_TAGLIB
QString fromTagLib(const TagLib::String &value)
{
    if (value.isEmpty())
        return {};
    return QString::fromUtf8(value.to8Bit(true).c_str());
}
#endif

} // namespace

QString MetadataReader::fallbackTitle(const QString &path)
{
    static const QRegularExpression leadingTrackNumber(
        QStringLiteral("^\\s*\\d{1,3}\\s*[-._)]\\s*"));

    const QFileInfo info(path);
    QString title = info.completeBaseName();
    title.remove(leadingTrackNumber);
    if (title.trimmed().isEmpty())
        title = info.completeBaseName();
    if (title.isEmpty())
        title = info.fileName();
    return title;
}

TrackMetadata MetadataReader::read(const QString &path, bool includeCover)
{
    TrackMetadata metadata;
    metadata.path = path;
    metadata.title = fallbackTitle(path);

#ifdef RESONANCE_HAS_TAGLIB
    TagLib::FileRef file(path.toUtf8().constData());
    if (file.isNull())
        return metadata;

    if (const TagLib::AudioProperties *properties = file.audioProperties())
        metadata.durationMs = properties->lengthInMilliseconds();

    const TagLib::Tag *tag = file.tag();
    if (!tag)
        return metadata;

    if (!tag->title().isEmpty())
        metadata.title = fromTagLib(tag->title());
    metadata.artist = fromTagLib(tag->artist());
    metadata.album = fromTagLib(tag->album());
    metadata.trackNumber = static_cast<int>(tag->track());
    metadata.year = static_cast<int>(tag->year());

    if (includeCover) {
        const auto pictures = file.complexProperties("PICTURE");
        for (const auto &picture : pictures) {
            TagLib::ByteVector data;
            QString mimeType;
            QString pictureType;

            for (auto it = picture.begin(); it != picture.end(); ++it) {
                const QString key = fromTagLib(it->first);
                if (key == QStringLiteral("data"))
                    data = it->second.toByteVector();
                else if (key == QStringLiteral("mimeType"))
                    mimeType = fromTagLib(it->second.toString());
                else if (key == QStringLiteral("pictureType"))
                    pictureType = fromTagLib(it->second.toString());
            }

            if (data.isEmpty())
                continue;

            // Prefer a front cover when the tag carries several images.
            if (metadata.hasCover()
                && !pictureType.contains(QStringLiteral("Front"),
                                        Qt::CaseInsensitive)) {
                continue;
            }

            metadata.coverData = QByteArray(
                data.data(), static_cast<int>(data.size()));
            metadata.coverMimeType = mimeType;
            if (pictureType.contains(QStringLiteral("Front"),
                                     Qt::CaseInsensitive)) {
                break;
            }
        }
    }
#else
    Q_UNUSED(includeCover)
#endif

    return metadata;
}

} // namespace resonance
