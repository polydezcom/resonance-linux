#include "ui/controllers/folder_tree_model.h"

#include <QtTest>

#include <QDir>
#include <QTemporaryDir>

using resonance::FolderTreeModel;

class TestFolderTree : public QObject
{
    Q_OBJECT

private slots:
    void buildsAndSortsRecursively();
    void expansionAndCurrentFolder();
    void folderNavigation();
    void collapseRootHidesChildren();
};

static QStringList nestedFiles(const QTemporaryDir &temp)
{
    const QString root = temp.path();
    return {
        root + QStringLiteral("/Album 10/10 - Last.flac"),
        root + QStringLiteral("/Album 2/Disc 1/1 - Nested.mp3"),
        root + QStringLiteral("/Album 2/2 - Second.mp3"),
        root + QStringLiteral("/Album 1/1 - First.wav"),
    };
}

void TestFolderTree::buildsAndSortsRecursively()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QStringList files = nestedFiles(temp);

    FolderTreeModel model;
    model.setRoot(temp.path(), files);

    QCOMPARE(model.count(), 4); // root, Album 1, Album 2, Album 10
    QCOMPARE(model.pathAt(0), temp.path());

    QCOMPARE(model.pathAt(1), temp.path() + QStringLiteral("/Album 1"));
    QCOMPARE(model.pathAt(2), temp.path() + QStringLiteral("/Album 2"));
    QCOMPARE(model.pathAt(3), temp.path() + QStringLiteral("/Album 10"));

    QCOMPARE(model.data(model.index(0), FolderTreeModel::TrackCountRole).toInt(),
             4);
    QCOMPARE(model.data(model.index(0), FolderTreeModel::CurrentRole).toBool(),
             true);
    QCOMPARE(model.data(model.index(2), FolderTreeModel::HasChildrenRole).toBool(),
             true);
}

void TestFolderTree::expansionAndCurrentFolder()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QStringList files = nestedFiles(temp);
    const QString album2 = temp.path() + QStringLiteral("/Album 2");
    const QString disc1 = album2 + QStringLiteral("/Disc 1");

    FolderTreeModel model;
    model.setRoot(temp.path(), files);
    QCOMPARE(model.rowForPath(disc1), -1);

    const int album2Row = model.rowForPath(album2);
    QVERIFY(album2Row >= 0);
    model.toggle(album2Row);
    QVERIFY(model.rowForPath(disc1) >= 0);

    model.setCurrentFolder(disc1);
    QCOMPARE(model.currentFolder(), disc1);
    const int row = model.rowForPath(disc1);
    QVERIFY(row >= 0);
    QCOMPARE(model.data(model.index(row), FolderTreeModel::CurrentRole).toBool(),
             true);
}

void TestFolderTree::folderNavigation()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QStringList files = nestedFiles(temp);
    const QString root = temp.path();
    const QString album1 = root + QStringLiteral("/Album 1");
    const QString album2 = root + QStringLiteral("/Album 2");
    const QString disc1 = album2 + QStringLiteral("/Disc 1");
    const QString album10 = root + QStringLiteral("/Album 10");

    FolderTreeModel model;
    model.setRoot(root, files);

    QCOMPARE(model.nextFolderPath(root), album1);
    QCOMPARE(model.nextFolderPath(album1), album2);
    QCOMPARE(model.nextFolderPath(album2), disc1);
    QCOMPARE(model.nextFolderPath(disc1), album10);
    QCOMPARE(model.nextFolderPath(album10), QString());

    QCOMPARE(model.previousFolderPath(album10), disc1);
    QCOMPARE(model.previousFolderPath(album1), root);
    QCOMPARE(model.previousFolderPath(root), QString());
}

void TestFolderTree::collapseRootHidesChildren()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    FolderTreeModel model;
    model.setRoot(temp.path(), nestedFiles(temp));

    QVERIFY(model.count() > 1);
    model.toggle(0); // collapse root
    QCOMPARE(model.count(), 1);
    QCOMPARE(model.pathAt(0), temp.path());
}

QTEST_GUILESS_MAIN(TestFolderTree)
#include "test_folder_tree.moc"
