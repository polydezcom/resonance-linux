#pragma once

#include <QString>

namespace resonance {

/**
 * Locale-aware natural ordering: "2 - Song" sorts before "10 - Song".
 * Numeric runs are compared by value, not by character.
 */
bool naturalLess(const QString &left, const QString &right);

} // namespace resonance
