#pragma once

#include <QFileInfo>
#include <QString>
#include <QStringList>

namespace resonance {

inline const QStringList &supportedAudioSuffixes()
{
    static const QStringList suffixes = {
        QStringLiteral("mp3"),
        QStringLiteral("flac"),
        QStringLiteral("wav"),
        QStringLiteral("ogg"),
        QStringLiteral("oga"),
    };
    return suffixes;
}

inline bool isSupportedAudioPath(const QString &path)
{
    return supportedAudioSuffixes().contains(QFileInfo(path).suffix().toLower());
}

} // namespace resonance
