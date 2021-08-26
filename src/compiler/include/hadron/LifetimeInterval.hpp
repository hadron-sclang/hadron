#ifndef SRC_COMPILER_INCLUDE_HADRON_LIFETIME_INTERVAL_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_LIFETIME_INTERVAL_HPP_

#include <cassert>
#include <list>
#include <set>

namespace hadron {

// The Linear Scan literature refers to a collection of LiveRanges as a single "lifetime interval," that may or may not
// contain "lifetime holes." These structs are named to respect that convention. We also include an in-order set of
// use times, as many linear scan heuristics order candidates by time of next use. No effort is made to enforce use
// time consistency with the LiveRange intervals.

// LiveRanges are currently [from, to) meaning usage is starting at |from| and ending, but not including, |to|.
struct LiveRange {
    LiveRange() = delete;
    LiveRange(size_t f, size_t t): from(f), to(t) {}
    ~LiveRange() = default;

    size_t from;
    size_t to;
};

struct LifetimeInterval {
    LifetimeInterval() = default;
    ~LifetimeInterval() = default;

    // Adds an interval in sorted order to list, possibly merging with other intervals.
    void addLiveRange(size_t from, size_t to);

    // Keeps all ranges before |splitTime|, return a new LifetimeInterval with all ranges after |splitTime|. If
    // |splitTime| is within a LiveRange it will also be split. Also splits the usages set.
    LifetimeInterval splitAt(size_t splitTime);

    // Returns true if p is within a LiveRange inside this LifetimeInterval.
    bool covers(size_t p) const;

    // Returns true if this and |lt| intersect, meaning there is some value |first| contained in a LiveRange for
    // both objects. Will set |first| to that value if true, will not modify first if false.
    bool findFirstIntersection(const LifetimeInterval& lt, size_t& first) const;

    bool isEmpty() const { return ranges.size() == 0; }
    size_t start() const { assert(!isEmpty()); return ranges.front().from; }
    size_t end() const { assert(!isEmpty()); return ranges.back().to; }

    std::list<LiveRange> ranges;
    std::set<size_t> usages;

    size_t valueNumber = 0;
    size_t registerNumber = 0;
    bool isSplit = false;
    bool isSpill = false;
    size_t spillSlot = 0;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_LIFETIME_INTERVAL_HPP_