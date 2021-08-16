#include "hadron/Lifetime.hpp"

namespace hadron {

void Lifetime::addInterval(size_t from, size_t to) {
    // Valid intervals only please.
    assert(to >= from);

    // Base case, interval list is empty.  TODO: maybe not needed.
    if (intervals.size() == 0) {
        intervals.emplace_back(Interval(from, to));
        return;
    }

    // Traverse existing interval list to see if |from| is contained in an existing interval.
    auto fromIter = intervals.end();
    bool fromWithin = false;
    auto toIter = intervals.end();
    bool toWithin = false;

    for (auto interval : intervals) {
        if (from < interval.from) {
            fromIter = interval;
        } else if (from <= interval.to) {
            // from >= interval.from && from <= interval.to means from is within this interval.
            fromIter = interval;
            fromWithin = true;
        }

        if (to < interval.from) {
            toIter = interval;
        } else if (to <= interval.to) {
            toIter = interval;
            toWithin = true;
        }

        // Stop searching
        if (to > interval.from) {
            break;
        }

    }
}

} // namespace hadron