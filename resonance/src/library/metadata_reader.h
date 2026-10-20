#pragma once

#include "library/track_metadata.h"

namespace resonance {

class MetadataReader
{
public:
    /// Reads tags and audio properties. Cover data is only loaded when
    /// includeCover is true; the list view avoids loading artwork for 10k files.
    static TrackMetadata read(const QString &path, bool includeCover = false);

    /// Filename fallback used when a file has no title tag.
    static QString fallbackTitle(const QString &path);
};

} // namespace resonance
