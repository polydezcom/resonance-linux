#include "library/natural_sort.h"

#include <QCollator>

namespace resonance {
namespace {

QCollator &collator()
{
    thread_local QCollator instance = [] {
        QCollator value;
        value.setNumericMode(true);
        value.setCaseSensitivity(Qt::CaseInsensitive);
        value.setIgnorePunctuation(false);
        return value;
    }();
    return instance;
}

} // namespace

bool naturalLess(const QString &left, const QString &right)
{
    if (left == right)
        return false;

    const int result = collator().compare(left, right);
    if (result != 0)
        return result < 0;

    // Guarantee a strict weak ordering when the locale considers two
    // different strings equivalent.
    return left < right;
}

} // namespace resonance
