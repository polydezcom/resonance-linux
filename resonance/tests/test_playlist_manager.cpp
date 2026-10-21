#include "library/playlist_manager.h"

#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

using resonance::PlaylistManager;

class TestPlaylistManager : public QObject
{
    Q_OBJECT

private slots:
    void createRenameAndPersist();
    void addMoveAndRemoveTracks();
    void uniqueNames();
    void malformedFileIsReported();
};

void TestPlaylistManager::createRenameAndPersist()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("playlists.json"));

    PlaylistManager manager(path);
    QVERIFY(manager.load());
    QVERIFY(manager.playlists().isEmpty());

    const QString id = manager.create(QStringLiteral("Road trip"));
    QVERIFY(!id.isEmpty());
    QCOMPARE(manager.name(id), QStringLiteral("Road trip"));

    QVERIFY(manager.rename(id, QStringLiteral("Night drive")));
    QVERIFY(manager.addTracks(id, {QStringLiteral("/music/a.flac"),
                                   QStringLiteral("/music/b.mp3")}));
    QVERIFY(manager.save());

    PlaylistManager loaded(path);
    QString error;
    QVERIFY2(loaded.load(&error), qPrintable(error));
    QCOMPARE(loaded.playlists().size(), 1);
    QCOMPARE(loaded.name(id), QStringLiteral("Night drive"));
    QCOMPARE(loaded.tracks(id), QStringList({QStringLiteral("/music/a.flac"),
                                             QStringLiteral("/music/b.mp3")}));
}

void TestPlaylistManager::addMoveAndRemoveTracks()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    PlaylistManager manager(temp.filePath(QStringLiteral("playlists.json")));
    const QString id = manager.create(QStringLiteral("List"));
    QVERIFY(manager.setTracks(id, {QStringLiteral("a"), QStringLiteral("b"),
                                   QStringLiteral("c"), QStringLiteral("d")}));

    QVERIFY(manager.moveTrack(id, 0, 2));
    QCOMPARE(manager.tracks(id), QStringList({QStringLiteral("b"),
                                              QStringLiteral("c"),
                                              QStringLiteral("a"),
                                              QStringLiteral("d")}));

    QVERIFY(manager.moveTrack(id, 3, 0));
    QCOMPARE(manager.tracks(id), QStringList({QStringLiteral("d"),
                                              QStringLiteral("b"),
                                              QStringLiteral("c"),
                                              QStringLiteral("a")}));

    QVERIFY(manager.removeTrack(id, 1));
    QCOMPARE(manager.tracks(id), QStringList({QStringLiteral("d"),
                                              QStringLiteral("c"),
                                              QStringLiteral("a")}));

    QVERIFY(!manager.moveTrack(id, 0, 0));
    QVERIFY(!manager.removeTrack(id, 99));
}

void TestPlaylistManager::uniqueNames()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    PlaylistManager manager(temp.filePath(QStringLiteral("playlists.json")));
    const QString first = manager.create(QStringLiteral("Mix"));
    const QString second = manager.create(QStringLiteral("Mix"));
    QCOMPARE(manager.name(first), QStringLiteral("Mix"));
    QCOMPARE(manager.name(second), QStringLiteral("Mix (2)"));
}

void TestPlaylistManager::malformedFileIsReported()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("broken.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("{ not json");
    file.close();

    PlaylistManager manager(path);
    QString error;
    QVERIFY(!manager.load(&error));
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(TestPlaylistManager)
#include "test_playlist_manager.moc"
