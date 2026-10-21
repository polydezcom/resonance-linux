#include "library/playlist_io.h"

#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

using resonance::PlaylistIO;

class TestPlaylistIO : public QObject
{
    Q_OBJECT

private slots:
    void m3u8RoundTrip();
    void plsRoundTrip();
    void readsRelativeEntries();
    void reportsMissingFile();
};

void TestPlaylistIO::m3u8RoundTrip()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QStringList tracks = {
        temp.filePath(QStringLiteral("01 - First.flac")),
        temp.filePath(QStringLiteral("02 - Second.mp3")),
    };
    for (const QString &track : tracks) {
        QFile file(track);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("audio");
    }

    const QString playlist = temp.filePath(QStringLiteral("mix.m3u8"));
    QString error;
    QVERIFY2(PlaylistIO::write(playlist, tracks, &error), qPrintable(error));
    QCOMPARE(PlaylistIO::read(playlist, &error), tracks);
    QVERIFY(error.isEmpty());

    QFile file(playlist);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray content = file.readAll();
    QVERIFY(content.startsWith("#EXTM3U\n"));
    QVERIFY(content.contains("01 - First.flac"));
    QVERIFY(!content.contains(temp.path().toUtf8()));
}

void TestPlaylistIO::plsRoundTrip()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QStringList tracks = {
        temp.filePath(QStringLiteral("one.ogg")),
        temp.filePath(QStringLiteral("two.wav")),
        temp.filePath(QStringLiteral("three.flac")),
    };
    for (const QString &track : tracks) {
        QFile file(track);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("audio");
    }

    const QString playlist = temp.filePath(QStringLiteral("mix.pls"));
    QString error;
    QVERIFY2(PlaylistIO::write(playlist, tracks, &error), qPrintable(error));
    QCOMPARE(PlaylistIO::read(playlist, &error), tracks);

    QFile file(playlist);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray content = file.readAll();
    QVERIFY(content.contains("[playlist]"));
    QVERIFY(content.contains("NumberOfEntries=3"));
}

void TestPlaylistIO::readsRelativeEntries()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString playlist = temp.filePath(QStringLiteral("relative.m3u"));
    QFile file(playlist);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("#EXTM3U\n#EXTINF:12,Some title\nsub/track.mp3\n/absolute/other.flac\n");
    file.close();

    const QStringList tracks = PlaylistIO::read(playlist);
    QCOMPARE(tracks.size(), 2);
    QCOMPARE(tracks.at(0), temp.filePath(QStringLiteral("sub/track.mp3")));
    QCOMPARE(tracks.at(1), QStringLiteral("/absolute/other.flac"));
}

void TestPlaylistIO::reportsMissingFile()
{
    QString error;
    const QStringList tracks = PlaylistIO::read(
        QStringLiteral("/definitely/does/not/exist.m3u"), &error);
    QVERIFY(tracks.isEmpty());
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(TestPlaylistIO)
#include "test_playlist_io.moc"
