#include "hadron/Lifetime.hpp"

namespace hadron {

void Lifetime::addInterval(size_t from, size_t to) {
    // Valid intervals only please.
    assert(to >= from);

    // Traverse existing interval list to see if |from| is contained in an existing interval.
    auto fromIter = intervals.begin();
    bool fromWithin = false;
    while (fromIter != intervals.end()) {
        if (from >= fromIter->from) {
            if (from < fromIter->to) {
                fromWithin = true;
                break;
            } else {
                ++fromIter;
            }
        } else {
            break;
        }
    }

    if (!fromWithin) {
        fromIter = intervals.emplace(fromIter, Interval(from, to));
    } else {
        // If re-using an existing interval expand to |to| if needed.
        fromIter->to = std::max(fromIter->to, to);
    }

    // fromIter is pointing at either the newly-created interval or the existing interval that contained |from|. Now
    // we iterate forward, deleting any intervals ending before |to| ends.
    auto toIter = fromIter;
    ++toIter;
    while (toIter != intervals.end()) {
        if (to >= toIter->to) {
            toIter = intervals.erase(toIter);
        } else if (to > toIter->from) {
            // The |to| is within an existing range, copy its later extent into ours before deletion.
            fromIter->to = toIter->to;
            intervals.erase(toIter);
            break;
        } else {
            break;
        }
    }
}

} // namespace hadron