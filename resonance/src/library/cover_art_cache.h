#pragma once

#include <QString>

namespace resonance {

class CoverArtCache
{
public:
    /// Returns a cache file path for the track's embedded or sidecar cover,
    /// or an empty string when there is no artwork. Safe to call off the UI
    /// thread; cache writes are best-effort.
    static QString coverForTrack(const QString &trackPath);
};

} // namespace resonance
