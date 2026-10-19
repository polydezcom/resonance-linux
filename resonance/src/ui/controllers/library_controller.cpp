#include "ui/controllers/library_controller.h"

#include "library/audio_formats.h"
#include "library/natural_sort.h"
#include "ui/controllers/app_settings.h"
#include "ui/controllers/player_controller.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>

namespace resonance {

LibraryController::LibraryController(PlayerController *player,
                                     AppSettings *settings, QObject *parent)
    : QObject(parent)
    , m_player(player)
    , m_settings(settings)
{
    connect(&m_scanner, &FolderScanner::scanStarted, this,
            [this](const QString &root) {
                setFolder(root);
                m_tracks.clear();
                emit tracksChanged();
            });
    connect(&m_scanner, &FolderScanner::scanFinished, this,
            &LibraryController::handleScanFinished);
    connect(&m_scanner, &FolderScanner::scanFailed, this,
            &LibraryController::handleScanFailed);
    connect(&m_scanner, &FolderScanner::scanningChanged, this,
            &LibraryController::scanningChanged);
}

QString LibraryController::folderName() const
{
    if (m_folder.isEmpty())
        return tr("No folder open");

    const QString name = QFileInfo(m_folder).fileName();
    return name.isEmpty() ? m_folder : name;
}

void LibraryController::setStartupFolder(const QString &path, bool autoPlay)
{
    m_startupFolder = path;
    m_startupAutoPlay = autoPlay;
}

void LibraryController::openFolder(const QUrl &url)
{
    clearError();
    m_autoPlay = true;
    m_scanner.scanUrl(url);
}

void LibraryController::openFolderPath(const QString &path, bool autoPlay)
{
    clearError();
    m_autoPlay = autoPlay;
    m_scanner.scanPath(path);
}

void LibraryController::openUrls(const QList<QUrl> &urls)
{
    clearError();

    QList<QUrl> localUrls;
    for (const QUrl &url : urls) {
        if (url.isLocalFile())
            localUrls.append(url);
    }

    if (localUrls.isEmpty()) {
        setError(tr("Only local files and folders are supported."));
        return;
    }

    for (const QUrl &url : localUrls) {
        const QFileInfo info(url.toLocalFile());
        if (info.isDir()) {
            openFolder(url);
            return;
        }
    }

    QStringList files;
    for (const QUrl &url : localUrls) {
        const QString path = url.toLocalFile();
        if (isSupportedAudioPath(path))
            files.append(path);
    }

    if (files.isEmpty()) {
        setError(tr("No supported audio files were dropped."));
        return;
    }

    std::sort(files.begin(), files.end(),
              [](const QString &left, const QString &right) {
                  return naturalLess(left, right);
              });

    const QString root = QFileInfo(files.first()).absolutePath();
    setFolder(root);
    m_tracks.setFiles(root, files);
    if (m_settings)
        m_settings->setLastFolder(root);
    emit tracksChanged();

    if (m_player)
        playIndex(0);
}

void LibraryController::playIndex(int index)
{
    if (!m_player)
        return;

    const QString path = m_tracks.pathAt(index);
    if (path.isEmpty())
        return;

    m_player->play(path, m_tracks.titleAt(index));
}

void LibraryController::refresh()
{
    if (m_folder.isEmpty()) {
        setError(tr("Open a folder first."));
        return;
    }
    openFolderPath(m_folder, false);
}

void LibraryController::clearError()
{
    setError({});
}

void LibraryController::handleScanFinished(const QString &root,
                                           const QStringList &files)
{
    setFolder(root);
    m_tracks.setFiles(root, files);
    if (m_settings)
        m_settings->setLastFolder(root);
    emit tracksChanged();

    if (files.isEmpty()) {
        setError(tr("No supported audio files were found in this folder."));
    } else {
        clearError();
    }

    const bool autoPlay = m_autoPlay;
    m_autoPlay = true;
    if (autoPlay && !files.isEmpty())
        playIndex(0);
}

void LibraryController::handleScanFailed(const QString &message)
{
    setError(message);
    m_autoPlay = true;
}

void LibraryController::setFolder(const QString &folder)
{
    if (m_folder == folder)
        return;
    m_folder = folder;
    emit folderChanged();
}

void LibraryController::setError(const QString &message)
{
    if (m_error == message)
        return;
    m_error = message;
    emit errorChanged();
}

} // namespace resonance
