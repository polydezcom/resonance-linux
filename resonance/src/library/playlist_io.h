#pragma once

#include <QString>
#include <QStringList>

namespace resonance {

class PlaylistIO
{
public:
    /// Reads M3U, M3U8 or PLS based on the file extension. Relative entries
    /// are resolved against the playlist file's directory.
    static QStringList read(const QString &filePath, QString *error = nullptr);

    /// Writes M3U8/M3U or PLS based on the target extension.
    static bool write(const QString &filePath, const QStringList &tracks,
                      QString *error = nullptr);

    static QStringList readM3U(const QString &filePath, QString *error = nullptr);
    static QStringList readPLS(const QString &filePath, QString *error = nullptr);
    static bool writeM3U(const QString &filePath, const QStringList &tracks,
                         QString *error = nullptr);
    static bool writePLS(const QString &filePath, const QStringList &tracks,
                         QString *error = nullptr);
};

} // namespace resonance
