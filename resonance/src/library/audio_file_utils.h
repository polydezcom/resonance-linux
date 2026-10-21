#pragma once

#include <QString>
#include <QStringList>

#include <atomic>

namespace resonance {

/// Collects supported audio files below a folder (or returns a single file),
/// skipping hidden entries and symlinks. Safe to call from a worker thread.
QStringList collectAudioFiles(const QString &path,
                              const std::atomic_bool *cancelled = nullptr);

} // namespace resonance
