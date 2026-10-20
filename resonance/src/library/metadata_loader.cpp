#include "library/metadata_loader.h"

#include "library/metadata_reader.h"

#include <QtConcurrent>

#include <algorithm>

namespace resonance {

MetadataLoader::MetadataLoader(QObject *parent)
    : QObject(parent)
{
    connect(&m_watcher, &QFutureWatcher<QVector<TrackMetadata>>::finished,
            this, [this] {
                if (m_batchGeneration != m_generation)
                    return;

                const QVector<TrackMetadata> batch = m_watcher.result();
                emit batchReady(m_generation, batch);

                m_nextIndex = m_batchEnd;
                if (m_nextIndex < m_files.size())
                    startNextBatch();
            });
}

void MetadataLoader::start(const QStringList &files)
{
    ++m_generation;
    m_files = files;
    m_nextIndex = 0;
    m_batchEnd = 0;
    m_batchGeneration = 0;

    if (m_files.isEmpty())
        return;
    startNextBatch();
}

void MetadataLoader::startNextBatch()
{
    const int begin = m_nextIndex;
    const int end = std::min(begin + BatchSize, static_cast<int>(m_files.size()));
    QStringList batch = m_files.mid(begin, end - begin);

    m_batchEnd = end;
    m_batchGeneration = m_generation;

    m_watcher.setFuture(QtConcurrent::run([batch = std::move(batch)] {
        QVector<TrackMetadata> result;
        result.reserve(batch.size());
        for (const QString &path : batch)
            result.append(MetadataReader::read(path, false));
        return result;
    }));
}

} // namespace resonance
