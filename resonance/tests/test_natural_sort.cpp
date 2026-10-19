#include "library/natural_sort.h"

#include <QtTest>

#include <algorithm>

using resonance::naturalLess;

class TestNaturalSort : public QObject
{
    Q_OBJECT

private slots:
    void numericRunsCompareByValue();
    void trackNumbers();
    void caseInsensitiveWithStableTieBreak();
    void sortsPathsNaturally();
    void strictWeakOrdering();
};

void TestNaturalSort::numericRunsCompareByValue()
{
    QVERIFY(naturalLess(QStringLiteral("2 - x"), QStringLiteral("10 - x")));
    QVERIFY(!naturalLess(QStringLiteral("10 - x"), QStringLiteral("2 - x")));
    QVERIFY(naturalLess(QStringLiteral("Disc 1"), QStringLiteral("Disc 2")));
}

void TestNaturalSort::trackNumbers()
{
    QVERIFY(naturalLess(QStringLiteral("01 - Intro"), QStringLiteral("2 - Theme")));
    QVERIFY(naturalLess(QStringLiteral("track 9"), QStringLiteral("track 10")));
    QVERIFY(naturalLess(QStringLiteral("track 099"), QStringLiteral("track 100")));
}

void TestNaturalSort::caseInsensitiveWithStableTieBreak()
{
    QVERIFY(naturalLess(QStringLiteral("alpha"), QStringLiteral("Beta")));
    QVERIFY(!naturalLess(QStringLiteral("same"), QStringLiteral("same")));

    QStringList values = {QStringLiteral("b"), QStringLiteral("A"),
                          QStringLiteral("a"), QStringLiteral("B")};
    std::sort(values.begin(), values.end(), naturalLess);
    QCOMPARE(values.at(0), QStringLiteral("A"));
    QCOMPARE(values.at(1), QStringLiteral("a"));
    QCOMPARE(values.at(2), QStringLiteral("B"));
    QCOMPARE(values.at(3), QStringLiteral("b"));
}

void TestNaturalSort::sortsPathsNaturally()
{
    QStringList paths = {
        QStringLiteral("/music/Album 10/10 - Last.flac"),
        QStringLiteral("/music/Album 2/2 - Second.flac"),
        QStringLiteral("/music/Album 1/1 - First.flac"),
        QStringLiteral("/music/Album 2/10 - Tenth.flac"),
        QStringLiteral("/music/Album 2/3 - Third.flac"),
    };

    std::sort(paths.begin(), paths.end(), naturalLess);

    const QStringList expected = {
        QStringLiteral("/music/Album 1/1 - First.flac"),
        QStringLiteral("/music/Album 2/2 - Second.flac"),
        QStringLiteral("/music/Album 2/3 - Third.flac"),
        QStringLiteral("/music/Album 2/10 - Tenth.flac"),
        QStringLiteral("/music/Album 10/10 - Last.flac"),
    };
    QCOMPARE(paths, expected);
}

void TestNaturalSort::strictWeakOrdering()
{
    const QStringList values = {
        QStringLiteral("a1"), QStringLiteral("a01"), QStringLiteral("a2"),
        QStringLiteral("A10"), QStringLiteral("a1b"), QStringLiteral("a1a"),
    };

    for (const QString &left : values) {
        QVERIFY(!naturalLess(left, left));
        for (const QString &right : values) {
            QVERIFY(!(naturalLess(left, right) && naturalLess(right, left)));
        }
    }
}

QTEST_GUILESS_MAIN(TestNaturalSort)
#include "test_natural_sort.moc"
