#pragma once

#include "library/track_metadata.h"

#include <QFutureWatcher>
#include <QObject>
#include <QStringList>
#include <QVector>

namespace resonance {

/**
 * Reads tags in small batches on the global thread pool and reports each
 * batch back on the UI thread. This keeps a 10k-track scan responsive without
 * a per-file signal storm.
 */
class MetadataLoader : public QObject
{
    Q_OBJECT

public:
    explicit MetadataLoader(QObject *parent = nullptr);

    void start(const QStringList &files);

signals:
    void batchReady(quint64 generation,
                    const QVector<resonance::TrackMetadata> &metadata);

private:
    void startNextBatch();

    static constexpr int BatchSize = 128;

    QFutureWatcher<QVector<TrackMetadata>> m_watcher;
    QStringList m_files;
    int m_nextIndex = 0;
    int m_batchEnd = 0;
    quint64 m_generation = 0;
    quint64 m_batchGeneration = 0;
};

} // namespace resonance
