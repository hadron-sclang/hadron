#include "hadron/LifetimeInterval.hpp"

namespace hadron {

void LifetimeInterval::addLiveRange(size_t from, size_t to) {
    // Valid ranges only please.
    assert(to >= from);

    // Traverse existing interval list to see if |from| is contained in an existing interval.
    auto fromIter = ranges.begin();
    bool fromWithin = false;
    while (fromIter != ranges.end()) {
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
        fromIter = ranges.emplace(fromIter, LiveRange(from, to));
    } else {
        // If re-using an existing interval expand to |to| if needed.
        fromIter->to = std::max(fromIter->to, to);
    }

    // fromIter is pointing at either the newly-created interval or the existing interval that contained |from|. Now
    // we iterate forward, deleting any intervals ending before |to| ends.
    auto toIter = fromIter;
    ++toIter;
    while (toIter != ranges.end()) {
        if (to >= toIter->to) {
            toIter = ranges.erase(toIter);
        } else if (to > toIter->from) {
            // The |to| is within an existing range, copy its later extent into ours before deletion.
            fromIter->to = toIter->to;
            ranges.erase(toIter);
            break;
        } else {
            break;
        }
    }
}

LifetimeInterval LifetimeInterval::splitAt(size_t splitTime) {
    LifetimeInterval split;
    split.valueNumber = valueNumber;
    split.isSplit = true;

    if (isEmpty() || end() <= splitTime) {
        return split;
    }

    if (splitTime <= start()) {
        split.ranges = std::move(ranges);
        ranges = std::list<LiveRange>();
        split.usages = std::move(usages);
        usages = std::set<size_t>();
        return split;
    }

    auto firstIter = ranges.begin();
    bool splitWithin = false;
    while (firstIter != ranges.end()) {
        if (firstIter->to <= splitTime) {
            ++firstIter;
        } else if (firstIter->from <= splitTime) {
            splitWithin = true;
            break;
        } else {
            break;
        }
    }

    // Transfer rest of list to the split lifetime.
    split.ranges.splice(split.ranges.end(), ranges, firstIter, ranges.end());
    if (splitWithin) {
        ranges.emplace_back(LiveRange(split.start(), splitTime));
        split.ranges.begin()->from = splitTime;
    }

    // Divide the usages sets.
    auto lowerBound = usages.lower_bound(splitTime);
    while (lowerBound != usages.end()) {
        split.usages.insert(usages.extract(lowerBound));
        lowerBound = usages.lower_bound(splitTime);
    }

    return split;
}

bool LifetimeInterval::covers(size_t p) const {
    if (isEmpty() || p < start() || p >= end()) {
        return false;
    }

    for (const auto& range : ranges) {
        if (p >= range.from && p < range.to) {
            return true;
        } else if (p < range.from) {
            return false;
        }
    }

    return false;
}

bool LifetimeInterval::findFirstIntersection(const LifetimeInterval& lt, size_t& first) const {
    // Early-out for either Interval empty.
    if (isEmpty() || lt.isEmpty()) {
        return false;
    }

    // Early-out for no intersection between the intervals.
    if (end() <= lt.start() || lt.end() <= start()) {
        return false;
    }

    auto a = ranges.begin();
    auto b = lt.ranges.begin();
    while (a != ranges.end() && b != lt.ranges.end()) {
        if (a->to <= b->from) {
            ++a;
        } else if (b->to <= a->from) {
            ++b;
        } else {
            first = std::max(a->from, b->from);
            return true;
        }
    }

    return false;
}

} // namespace hadron