#include "library/folder_scanner.h"

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>

using resonance::FolderScanner;

class TestFolderScanner : public QObject
{
    Q_OBJECT

private slots:
    void scansRecursivelyAndSortsNaturally();
    void ignoresUnsupportedAndHiddenFiles();
    void reportsMissingFolder();
    void emptyFolderFinishesWithNoFiles();

private:
    static void touch(const QString &path)
    {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("not real audio");
    }
};

void TestFolderScanner::scansRecursivelyAndSortsNaturally()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    touch(temp.filePath(QStringLiteral("Album 10/10 - Last.flac")));
    touch(temp.filePath(QStringLiteral("Album 2/10 - Tenth.ogg")));
    touch(temp.filePath(QStringLiteral("Album 2/2 - Second.mp3")));
    touch(temp.filePath(QStringLiteral("Album 1/1 - First.wav")));

    FolderScanner scanner;
    QSignalSpy spy(&scanner, &FolderScanner::scanFinished);
    scanner.scanPath(temp.path());

    QVERIFY2(spy.wait(5000), "scanner did not finish");
    const QList<QVariant> args = spy.takeFirst();
    const QStringList files = args.at(1).toStringList();

    QCOMPARE(files.size(), 4);
    QVERIFY(files.at(0).endsWith(QStringLiteral("Album 1/1 - First.wav")));
    QVERIFY(files.at(1).endsWith(QStringLiteral("Album 2/2 - Second.mp3")));
    QVERIFY(files.at(2).endsWith(QStringLiteral("Album 2/10 - Tenth.ogg")));
    QVERIFY(files.at(3).endsWith(QStringLiteral("Album 10/10 - Last.flac")));
}

void TestFolderScanner::ignoresUnsupportedAndHiddenFiles()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    touch(temp.filePath(QStringLiteral("visible.mp3")));
    touch(temp.filePath(QStringLiteral("notes.txt")));
    touch(temp.filePath(QStringLiteral(".hidden.mp3")));
    touch(temp.filePath(QStringLiteral(".hidden/secret.flac")));

    FolderScanner scanner;
    QSignalSpy spy(&scanner, &FolderScanner::scanFinished);
    scanner.scanPath(temp.path());

    QVERIFY2(spy.wait(5000), "scanner did not finish");
    const QStringList files = spy.takeFirst().at(1).toStringList();
    QCOMPARE(files.size(), 1);
    QVERIFY(files.first().endsWith(QStringLiteral("visible.mp3")));
}

void TestFolderScanner::reportsMissingFolder()
{
    FolderScanner scanner;
    QSignalSpy spy(&scanner, &FolderScanner::scanFailed);
    scanner.scanPath(QStringLiteral("/definitely/not/a/real/folder"));

    QCOMPARE(spy.count(), 1);
    QVERIFY(!scanner.isScanning());
}

void TestFolderScanner::emptyFolderFinishesWithNoFiles()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    FolderScanner scanner;
    QSignalSpy spy(&scanner, &FolderScanner::scanFinished);
    scanner.scanPath(temp.path());

    QVERIFY2(spy.wait(5000), "scanner did not finish");
    QVERIFY(spy.takeFirst().at(1).toStringList().isEmpty());
}

QTEST_GUILESS_MAIN(TestFolderScanner)
#include "test_folder_scanner.moc"
