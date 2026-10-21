#include "library/audio_file_utils.h"

#include "library/audio_formats.h"
#include "library/natural_sort.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <algorithm>

namespace resonance {

QStringList collectAudioFiles(const QString &path,
                              const std::atomic_bool *cancelled)
{
    QStringList files;
    const QFileInfo info(path);

    if (info.isFile()) {
        if (isSupportedAudioPath(path))
            files.append(info.absoluteFilePath());
        return files;
    }
    if (!info.isDir())
        return files;

    const QString root = info.canonicalFilePath().isEmpty()
        ? info.absoluteFilePath() : info.canonicalFilePath();

    QDirIterator iterator(root,
                          QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        if (cancelled && cancelled->load(std::memory_order_relaxed))
            break;
        const QString candidate = iterator.next();
        if (isSupportedAudioPath(candidate))
            files.append(candidate);
    }

    std::sort(files.begin(), files.end(),
              [](const QString &left, const QString &right) {
                  return naturalLess(left, right);
              });
    return files;
}

} // namespace resonance
