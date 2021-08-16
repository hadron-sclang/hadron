#include "hadron/Lifetime.hpp"

namespace hadron {

void Lifetime::addInterval(size_t from, size_t to) {
    // Valid intervals only please.
    assert(to >= from);

    // Traverse existing interval list to see if |from| is contained in an existing interval.
    auto fromIter = intervals.end();
    bool fromWithin = false;
    auto toIter = intervals.end();
    bool toWithin = false;

    for (auto interval = intervals.begin(); interval != intervals.end(); ++interval) {
        if (from < interval->from) {
            fromIter = interval;
        } else if (from <= interval->to) {
            // from >= interval.from && from <= interval.to means from is within this interval.
            fromIter = interval;
            fromWithin = true;
        }

        if (to < interval->from) {
            toIter = interval;
        } else if (to <= interval->to) {
            toIter = interval;
            toWithin = true;
        }

        // Stop searching if we're past the end of the 'to' value.
        if (to > interval->from) {
            break;
        }
    }

    if (!fromWithin) {
        fromIter = intervals.emplace(fromIter, Interval(from, to));
    }

    // Expand from range to end of existing to range.
    if (toWithin) {
        fromIter->to = toIter->to;
    }

    // Remove any elements between fromIter and toIter that may have been subsumed into this new range.
    if (fromIter != toIter) {
        ++fromIter;
        while (fromIter != toIter) {
            fromIter = intervals.erase(fromIter);
        }
    }
}

} // namespace hadron