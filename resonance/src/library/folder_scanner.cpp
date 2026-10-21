#include "library/folder_scanner.h"

#include "library/audio_file_utils.h"

#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrent>

namespace resonance {

FolderScanner::FolderScanner(QObject *parent)
    : QObject(parent)
{
}

FolderScanner::~FolderScanner()
{
    if (m_cancelToken)
        m_cancelToken->store(true);
}

void FolderScanner::scanUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        emit scanFailed(tr("Only local folders are supported."));
        return;
    }
    scanPath(url.toLocalFile());
}

void FolderScanner::scanPath(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        emit scanFailed(tr("Folder does not exist: %1").arg(path));
        return;
    }
    if (!info.isDir()) {
        emit scanFailed(tr("Not a folder: %1").arg(path));
        return;
    }

    QString root = info.canonicalFilePath();
    if (root.isEmpty())
        root = info.absoluteFilePath();

    beginScan(root);
}

void FolderScanner::beginScan(const QString &root)
{
    if (m_cancelToken)
        m_cancelToken->store(true);

    auto token = std::make_shared<std::atomic_bool>(false);
    m_cancelToken = token;
    const quint64 generation = ++m_generation;

    m_root = root;
    emit rootChanged();
    setScanning(true);
    emit scanStarted(root);

    auto *watcher = new QFutureWatcher<QStringList>(this);
    connect(watcher, &QFutureWatcher<QStringList>::finished, this,
            [this, watcher, root, generation, token] {
                const QStringList files = watcher->result();
                watcher->deleteLater();

                if (generation != m_generation)
                    return;

                setScanning(false);
                if (token->load(std::memory_order_relaxed)) {
                    emit scanFailed(tr("Scan cancelled."));
                    return;
                }
                emit scanFinished(root, files);
            });

    watcher->setFuture(QtConcurrent::run(
        [root, token] { return collectAudioFiles(root, token.get()); }));
}

void FolderScanner::setScanning(bool scanning)
{
    if (m_scanning == scanning)
        return;
    m_scanning = scanning;
    emit scanningChanged();
}

} // namespace resonance
