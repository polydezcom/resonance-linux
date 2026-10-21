#include "library/playlist_io.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QObject>
#include <QSaveFile>
#include <QUrl>

namespace resonance {
namespace {

QString resolveEntry(const QString &entry, const QDir &base)
{
    QString value = entry.trimmed();
    if (value.isEmpty())
        return {};

    if (value.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive))
        return QUrl(value).toLocalFile();

    if (QDir::isAbsolutePath(value))
        return QDir::cleanPath(value);

    return QDir::cleanPath(base.absoluteFilePath(value));
}

QString exportPath(const QString &track, const QDir &base)
{
    const QString relative = base.relativeFilePath(track);
    return QDir::fromNativeSeparators(relative);
}

QString displayTitle(const QString &path)
{
    return QFileInfo(path).completeBaseName();
}

bool writeText(const QString &filePath, const QString &text, QString *error)
{
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = QObject::tr("Could not write playlist: %1")
                         .arg(file.errorString());
        return false;
    }
    file.write(text.toUtf8());
    if (!file.commit()) {
        if (error)
            *error = QObject::tr("Could not save playlist: %1")
                         .arg(file.errorString());
        return false;
    }
    return true;
}

} // namespace

QStringList PlaylistIO::read(const QString &filePath, QString *error)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QStringLiteral("pls"))
        return readPLS(filePath, error);
    if (suffix == QStringLiteral("m3u") || suffix == QStringLiteral("m3u8")
        || suffix.isEmpty()) {
        return readM3U(filePath, error);
    }
    if (error)
        *error = QObject::tr("Unsupported playlist format: .%1").arg(suffix);
    return {};
}

bool PlaylistIO::write(const QString &filePath, const QStringList &tracks,
                       QString *error)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QStringLiteral("pls"))
        return writePLS(filePath, tracks, error);
    return writeM3U(filePath, tracks, error);
}

QStringList PlaylistIO::readM3U(const QString &filePath, QString *error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QObject::tr("Could not open playlist: %1")
                         .arg(file.errorString());
        return {};
    }

    QString content = QString::fromUtf8(file.readAll());
    if (content.startsWith(QChar(0xfeff)))
        content.remove(0, 1);

    const QDir base = QFileInfo(filePath).dir();
    QStringList tracks;
    const QStringList lines = content.split(QLatin1Char('\n'));
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        const QString path = resolveEntry(line, base);
        if (!path.isEmpty())
            tracks.append(path);
    }
    return tracks;
}

QStringList PlaylistIO::readPLS(const QString &filePath, QString *error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QObject::tr("Could not open playlist: %1")
                         .arg(file.errorString());
        return {};
    }

    QMap<int, QString> entries;
    const QDir base = QFileInfo(filePath).dir();
    const QStringList lines =
        QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));

    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        const int equals = line.indexOf(QLatin1Char('='));
        if (equals <= 0)
            continue;

        const QString key = line.left(equals).trimmed().toLower();
        if (!key.startsWith(QStringLiteral("file")))
            continue;

        bool ok = false;
        const int number = key.mid(4).toInt(&ok);
        if (!ok || number <= 0)
            continue;

        const QString path = resolveEntry(line.mid(equals + 1), base);
        if (!path.isEmpty())
            entries.insert(number, path);
    }

    QStringList tracks;
    tracks.reserve(entries.size());
    for (const QString &path : entries)
        tracks.append(path);
    return tracks;
}

bool PlaylistIO::writeM3U(const QString &filePath, const QStringList &tracks,
                          QString *error)
{
    const QDir base = QFileInfo(filePath).dir();
    QString content = QStringLiteral("#EXTM3U\n");
    for (const QString &track : tracks) {
        content += QStringLiteral("#EXTINF:-1,%1\n").arg(displayTitle(track));
        content += exportPath(track, base);
        content += QLatin1Char('\n');
    }
    return writeText(filePath, content, error);
}

bool PlaylistIO::writePLS(const QString &filePath, const QStringList &tracks,
                          QString *error)
{
    const QDir base = QFileInfo(filePath).dir();
    QString content = QStringLiteral("[playlist]\n");
    int number = 1;
    for (const QString &track : tracks) {
        content += QStringLiteral("File%1=%2\n")
                       .arg(number)
                       .arg(exportPath(track, base));
        content += QStringLiteral("Title%1=%2\n").arg(number).arg(displayTitle(track));
        content += QStringLiteral("Length%1=-1\n").arg(number);
        ++number;
    }
    content += QStringLiteral("NumberOfEntries=%1\n").arg(tracks.size());
    content += QStringLiteral("Version=2\n");
    return writeText(filePath, content, error);
}

} // namespace resonance
