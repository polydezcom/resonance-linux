#include "core/model/play_queue.h"

#include <algorithm>
#include <filesystem>
#include <numeric>

namespace resonance {

void PlayQueue::setItems(std::vector<std::string> items)
{
    m_items = std::move(items);
    m_current = -1;
    m_order.clear();
    m_orderPosition = 0;
}

std::string PlayQueue::currentPath() const
{
    if (m_current < 0 || m_current >= size())
        return {};
    return m_items[static_cast<std::size_t>(m_current)];
}

void PlayQueue::setCurrentIndex(int index)
{
    if (index < 0 || index >= size()) {
        m_current = -1;
        m_order.clear();
        m_orderPosition = 0;
        return;
    }

    m_current = index;
    if (m_shuffle)
        rebuildShuffleOrder();
}

int PlayQueue::advance(bool userInitiated)
{
    if (m_current < 0 || isEmpty())
        return -1;

    if (m_repeat == RepeatMode::Track && !userInitiated)
        return m_current;

    if (m_shuffle) {
        ensureShufflePosition();
        if (m_orderPosition + 1 < m_order.size()) {
            ++m_orderPosition;
            m_current = m_order[m_orderPosition];
            return m_current;
        }
        if (m_repeat == RepeatMode::Folder) {
            m_orderPosition = 0;
            m_current = m_order.front();
            return m_current;
        }
        return -1;
    }

    if (m_current + 1 < size()) {
        ++m_current;
        return m_current;
    }
    if (m_repeat == RepeatMode::Folder) {
        m_current = 0;
        return m_current;
    }
    return -1;
}

int PlayQueue::rewind(bool userInitiated)
{
    if (m_current < 0 || isEmpty())
        return -1;

    if (m_repeat == RepeatMode::Track && !userInitiated)
        return m_current;

    if (m_shuffle) {
        ensureShufflePosition();
        if (m_orderPosition > 0) {
            --m_orderPosition;
            m_current = m_order[m_orderPosition];
            return m_current;
        }
        if (m_repeat == RepeatMode::Folder) {
            m_orderPosition = m_order.size() - 1;
            m_current = m_order[m_orderPosition];
            return m_current;
        }
        return -1;
    }

    if (m_current > 0) {
        --m_current;
        return m_current;
    }
    if (m_repeat == RepeatMode::Folder) {
        m_current = size() - 1;
        return m_current;
    }
    return -1;
}

int PlayQueue::peekAdvance(bool userInitiated) const
{
    return peekAdvanceFrom(m_current, userInitiated);
}

int PlayQueue::peekRewind(bool userInitiated) const
{
    return peekRewindFrom(m_current, userInitiated);
}

int PlayQueue::peekAdvanceFrom(int index, bool userInitiated) const
{
    if (index < 0 || index >= size() || isEmpty())
        return -1;

    if (m_repeat == RepeatMode::Track && !userInitiated)
        return index;

    if (m_shuffle && m_order.size() == m_items.size()) {
        const auto it = std::find(m_order.begin(), m_order.end(), index);
        if (it != m_order.end()) {
            const auto position = static_cast<std::size_t>(
                std::distance(m_order.begin(), it));
            if (position + 1 < m_order.size())
                return m_order[position + 1];
            if (m_repeat == RepeatMode::Folder)
                return m_order.front();
            return -1;
        }
    }

    if (index + 1 < size())
        return index + 1;
    if (m_repeat == RepeatMode::Folder)
        return 0;
    return -1;
}

int PlayQueue::peekRewindFrom(int index, bool userInitiated) const
{
    if (index < 0 || index >= size() || isEmpty())
        return -1;

    if (m_repeat == RepeatMode::Track && !userInitiated)
        return index;

    if (m_shuffle && m_order.size() == m_items.size()) {
        const auto it = std::find(m_order.begin(), m_order.end(), index);
        if (it != m_order.end()) {
            const auto position = static_cast<std::size_t>(
                std::distance(m_order.begin(), it));
            if (position > 0)
                return m_order[position - 1];
            if (m_repeat == RepeatMode::Folder)
                return m_order.back();
            return -1;
        }
    }

    if (index > 0)
        return index - 1;
    if (m_repeat == RepeatMode::Folder)
        return size() - 1;
    return -1;
}

void PlayQueue::setShuffle(bool shuffle)
{
    if (m_shuffle == shuffle)
        return;

    m_shuffle = shuffle;
    if (m_shuffle)
        rebuildShuffleOrder();
    else {
        m_order.clear();
        m_orderPosition = 0;
    }
}

std::string PlayQueue::folderForPath(const std::string &path)
{
    if (path.empty())
        return {};
    return std::filesystem::path(path).parent_path().lexically_normal().string();
}

void PlayQueue::rebuildShuffleOrder()
{
    m_order.clear();
    m_order.reserve(m_items.size());

    if (m_current >= 0)
        m_order.push_back(m_current);

    std::vector<int> rest;
    rest.reserve(m_items.size());
    for (int i = 0; i < size(); ++i) {
        if (i != m_current)
            rest.push_back(i);
    }
    std::shuffle(rest.begin(), rest.end(), m_rng);
    m_order.insert(m_order.end(), rest.begin(), rest.end());
    m_orderPosition = 0;
}

void PlayQueue::ensureShufflePosition()
{
    if (m_order.empty() || m_order.size() != m_items.size()) {
        rebuildShuffleOrder();
        return;
    }

    const auto it = std::find(m_order.begin(), m_order.end(), m_current);
    if (it == m_order.end()) {
        rebuildShuffleOrder();
        return;
    }
    m_orderPosition = static_cast<std::size_t>(std::distance(m_order.begin(), it));
}

} // namespace resonance
