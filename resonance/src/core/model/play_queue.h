#pragma once

#include <cstddef>
#include <random>
#include <string>
#include <vector>

namespace resonance {

enum class RepeatMode {
    Off,
    Track,
    Folder,
};

/**
 * Pure playback order logic. The engine owns one of these; the UI never
 * mutates it while the decoder thread is reading without going through the
 * engine's mutex.
 *
 * "Folder" repeat wraps the current queue. In M2 the queue is always the
 * recursive contents of one selected folder.
 */
class PlayQueue
{
public:
    void setItems(std::vector<std::string> items);
    const std::vector<std::string> &items() const { return m_items; }
    int size() const { return static_cast<int>(m_items.size()); }
    bool isEmpty() const { return m_items.empty(); }

    int currentIndex() const { return m_current; }
    std::string currentPath() const;
    void setCurrentIndex(int index);

    /// Auto-advance (repeat-track respected) or user-initiated next.
    int advance(bool userInitiated = false);
    int rewind(bool userInitiated = false);

    /// Non-mutating variants used by the engine before it commits to a
    /// user-initiated jump. The From variants let navigation follow the
    /// audible track even when the decoder has already prefetched ahead.
    int peekAdvance(bool userInitiated = false) const;
    int peekRewind(bool userInitiated = false) const;
    int peekAdvanceFrom(int index, bool userInitiated = false) const;
    int peekRewindFrom(int index, bool userInitiated = false) const;

    void setShuffle(bool shuffle);
    bool shuffle() const { return m_shuffle; }

    void setRepeatMode(RepeatMode mode) { m_repeat = mode; }
    RepeatMode repeatMode() const { return m_repeat; }

    static std::string folderForPath(const std::string &path);

private:
    void rebuildShuffleOrder();
    void ensureShufflePosition();

    std::vector<std::string> m_items;
    int m_current = -1;
    bool m_shuffle = false;
    RepeatMode m_repeat = RepeatMode::Off;

    std::vector<int> m_order;
    std::size_t m_orderPosition = 0;
    std::mt19937 m_rng{std::random_device{}()};
};

} // namespace resonance
