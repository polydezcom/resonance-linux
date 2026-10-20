#include "core/model/play_queue.h"

#include <QtTest>

#include <algorithm>
#include <set>
#include <vector>

using resonance::PlayQueue;
using resonance::RepeatMode;

class TestPlayQueue : public QObject
{
    Q_OBJECT

private slots:
    void naturalAdvanceAndRewind();
    void repeatOffStopsAtEnds();
    void repeatFolderWraps();
    void repeatTrackStaysPut();
    void userNextSkipsRepeatTrack();
    void shuffleVisitsEveryTrackOnce();
    void shuffleRespectsCurrentOnToggle();
    void folderForPath();
};

static PlayQueue makeQueue()
{
    PlayQueue queue;
    queue.setItems({"a/1.mp3", "a/2.mp3", "b/1.mp3", "b/2.mp3"});
    queue.setCurrentIndex(0);
    return queue;
}

void TestPlayQueue::naturalAdvanceAndRewind()
{
    PlayQueue queue = makeQueue();
    QCOMPARE(queue.advance(), 1);
    QCOMPARE(queue.advance(), 2);
    QCOMPARE(queue.advance(), 3);
    QCOMPARE(queue.rewind(), 2);
    QCOMPARE(queue.peekAdvance(), 3);
    QCOMPARE(queue.currentIndex(), 2); // peek does not mutate
}

void TestPlayQueue::repeatOffStopsAtEnds()
{
    PlayQueue queue = makeQueue();
    queue.setCurrentIndex(3);
    QCOMPARE(queue.advance(), -1);
    QCOMPARE(queue.currentIndex(), 3);

    queue.setCurrentIndex(0);
    QCOMPARE(queue.rewind(), -1);
    QCOMPARE(queue.currentIndex(), 0);
}

void TestPlayQueue::repeatFolderWraps()
{
    PlayQueue queue = makeQueue();
    queue.setRepeatMode(RepeatMode::Folder);
    queue.setCurrentIndex(3);
    QCOMPARE(queue.advance(), 0);
    QCOMPARE(queue.rewind(), 3);

    queue.setShuffle(false);
    QCOMPARE(queue.peekAdvance(), 0); // from 3, wraps to 0
}

void TestPlayQueue::repeatTrackStaysPut()
{
    PlayQueue queue = makeQueue();
    queue.setRepeatMode(RepeatMode::Track);
    queue.setCurrentIndex(2);
    QCOMPARE(queue.advance(), 2);
    QCOMPARE(queue.advance(false), 2);
    QCOMPARE(queue.rewind(false), 2);
}

void TestPlayQueue::userNextSkipsRepeatTrack()
{
    PlayQueue queue = makeQueue();
    queue.setRepeatMode(RepeatMode::Track);
    queue.setCurrentIndex(2);
    QCOMPARE(queue.advance(true), 3);
    QCOMPARE(queue.rewind(true), 2);
}

void TestPlayQueue::shuffleVisitsEveryTrackOnce()
{
    PlayQueue queue = makeQueue();
    queue.setCurrentIndex(1);
    queue.setShuffle(true);

    std::set<int> visited;
    visited.insert(queue.currentIndex());
    for (int i = 0; i < queue.size() - 1; ++i) {
        const int next = queue.advance();
        QVERIFY(next >= 0);
        visited.insert(next);
    }
    QCOMPARE(visited.size(), std::size_t(queue.size()));
}

void TestPlayQueue::shuffleRespectsCurrentOnToggle()
{
    PlayQueue queue = makeQueue();
    queue.setCurrentIndex(2);
    queue.setShuffle(true);
    QCOMPARE(queue.currentIndex(), 2);

    const int next = queue.advance();
    QVERIFY(next != 2);
    QVERIFY(next >= 0 && next < queue.size());

    queue.setShuffle(false);
    QCOMPARE(queue.currentIndex(), next);
}

void TestPlayQueue::folderForPath()
{
    QCOMPARE(PlayQueue::folderForPath("/music/Album 1/01 - Song.flac"),
             std::string("/music/Album 1"));
    QCOMPARE(PlayQueue::folderForPath("relative/song.mp3"),
             std::string("relative"));
    QCOMPARE(PlayQueue::folderForPath(""), std::string());
}

QTEST_APPLESS_MAIN(TestPlayQueue)
#include "test_play_queue.moc"
