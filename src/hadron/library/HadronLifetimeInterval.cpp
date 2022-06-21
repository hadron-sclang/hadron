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
    auto split = LifetimeInterval::makeLifetimeInterval(context, valueNumber().int32());
    split.setIsSplit(Boolean(true));

    if (isEmpty() || end().int32() <= splitTime) {
        return split;
    }

    // If splitting before the start just move our members to split and empty out ourselves.
    if (splitTime <= start().int32()) {
        split.setRanges(ranges());
        setRanges(TypedArray<LiveRange>());
        split.setUsages(usages());
        setUsages(OrderedIdentitySet::makeIdentitySet(context));
    }

    #error keep going here

    return split;
}

bool LifetimeInterval::covers(int32_t p) const {
    return false;
}

bool LifetimeInterval::findFirstIntersection(const LifetimeInterval lt, int32_t& first) const {
    return false;
}

} // namespace library
} // namespace hadron