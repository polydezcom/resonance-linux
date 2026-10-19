#pragma once

#include <QObject>
#include <QStringList>
#include <QUrl>

#include <atomic>
#include <memory>

namespace resonance {

/**
 * Recursive, path-only folder scanner. It deliberately does not read tags:
 * scanning a 10k-track folder stays fast, and metadata is filled in lazily by
 * a later milestone.
 */
class FolderScanner : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(QString root READ root NOTIFY rootChanged)

public:
    explicit FolderScanner(QObject *parent = nullptr);
    ~FolderScanner() override;

    Q_INVOKABLE void scanUrl(const QUrl &url);
    Q_INVOKABLE void scanPath(const QString &path);

    bool isScanning() const { return m_scanning; }
    QString root() const { return m_root; }

signals:
    void scanStarted(const QString &root);
    void scanFinished(const QString &root, const QStringList &files);
    void scanFailed(const QString &error);
    void scanningChanged();
    void rootChanged();

private:
    void beginScan(const QString &root);
    void setScanning(bool scanning);

    bool m_scanning = false;
    QString m_root;
    quint64 m_generation = 0;
    std::shared_ptr<std::atomic_bool> m_cancelToken;
};

} // namespace resonance
