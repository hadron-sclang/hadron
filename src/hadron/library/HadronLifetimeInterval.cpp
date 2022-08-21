#include "hadron/library/HadronLifetimeInterval.hpp"

namespace hadron {
namespace library {

void LifetimeInterval::addLiveRange(ThreadContext* context, int32_t from, int32_t to) {
    // Valid ranges only please.
    assert(to >= from);

    // Traverse existing interval list to see if |from| is contained in an existing interval.
    int32_t fromIndex = 0;
    auto fromRange = LiveRange();
    bool fromWithin = false;
    while (fromIndex < ranges().size()) {
        fromRange = ranges().typedAt(fromIndex);
        if (from >= fromRange.from().int32()) {
            if (from < fromRange.to().int32()) {
                fromWithin = true;
                break;
            } else {
                ++fromIndex;
            }
        } else {
            break;
        }
    }

    if (!fromWithin) {
        fromRange = LiveRange::makeLiveRange(context, from, to);
        setRanges(ranges().typedInsert(context, fromIndex, fromRange));
    } else {
        fromRange.setTo(Integer(std::max(fromRange.to().int32(), to)));
    }

    // fromIndex is at either the newly-created interval or the existing interval that contained |from|. Now
    // we iterate forward, deleting any intervals ending before |to| ends.
    auto toIndex = fromIndex + 1;
    while (toIndex < ranges().size()) {
        auto toRange = ranges().typedAt(toIndex);
        if (to >= toRange.to().int32()) {
            ranges().removeAt(context, toIndex);
        } else if (to > toRange.from().int32()) {
            // The |to| is within an existing range, copy its later extent into ours before deletion.
            fromRange.setTo(toRange.to());
            ranges().removeAt(context, toIndex);
            break;
        } else {
            break;
        }
    }
}

LifetimeInterval LifetimeInterval::splitAt(ThreadContext* context, int32_t splitTime) {
    auto split = LifetimeInterval::makeLifetimeInterval(context, valueNumber());
    split.setIsSplit(true);

    if (isEmpty() || end().int32() <= splitTime) {
        return split;
    }

    // If splitting before the start just move our members to split and empty out ourselves.
    if (splitTime <= start().int32()) {
        split.setRanges(ranges());
        setRanges(TypedArray<LiveRange>());
        split.setUsages(usages());
        setUsages(OrderedIdentitySet::makeIdentitySet(context));
        return split;
    }

    int32_t firstIndex = 0;
    bool splitWithin = false;
    while (firstIndex < ranges().size()) {
        auto firstRange = ranges().typedAt(firstIndex);
        if (firstRange.to().int32() <= splitTime) {
            ++firstIndex;
        } else if (firstRange.from().int32() <= splitTime) {
            splitWithin = true;
            break;
        } else {
            break;
        }
    }

    // Transfer rest of list to the split lifetime.
    split.setRanges(ranges().typedCopyRange(context, firstIndex, ranges().size() - 1));
    ranges().resize(context, firstIndex);
    if (splitWithin) {
        setRanges(ranges().typedAdd(context, LiveRange::makeLiveRange(context, split.start().int32(), splitTime)));
        split.ranges().typedFirst().setFrom(Integer(splitTime));
    }

    // Divide the usages sets.
    auto lowerBound = usages().lowerBound(Integer(splitTime));
    while (lowerBound) {
        split.usages().add(context, lowerBound.slot());
        usages().remove(context, lowerBound.slot());
        lowerBound = usages().lowerBound(Integer(splitTime));
    }

    return split;
}

bool LifetimeInterval::covers(int32_t p) const {
    if (isEmpty() || p < start().int32() || p >= end().int32()) {
        return false;
    }

    for (int32_t i = 0; i < ranges().size(); ++i) {
        auto range = ranges().typedAt(i);
        if (range.from().int32() <= p && p < range.to().int32()) {
            return true;
        } else if (p < range.from().int32()) {
            return false;
        }
    }

    return false;
}

bool LifetimeInterval::findFirstIntersection(const LifetimeInterval lt, int32_t& first) const {
    // Early-out for either Interval empty.
    if (isEmpty() || lt.isEmpty()) {
        return false;
    }

    // Early-out for no intersection between the intervals.
    if (end().int32() <= lt.start().int32() || lt.end().int32() <= start().int32()) {
        return false;
    }

    int32_t aIndex = 0;
    int32_t bIndex = 0;
    auto a = ranges().typedAt(0);
    auto b = lt.ranges().typedAt(0);
    while (aIndex < ranges().size() && bIndex < lt.ranges().size()) {
        if (a.to().int32() <= b.from().int32()) {
            // If a ends before b begins, advance to next a.
            ++aIndex;
            if (aIndex < ranges().size()) { a = ranges().typedAt(aIndex); }
        } else if (b.to().int32() <= a.from().int32()) {
            // If b ends before a begins, advance to next b.
            ++bIndex;
            if (bIndex < lt.ranges().size()) { b = lt.ranges().typedAt(bIndex); }
        } else {
            // Intersection is after both a and b have started.
            first = std::max(a.from().int32(), b.from().int32());
            return true;
        }
    }

    return false;
}

} // namespace library
} // namespace hadron