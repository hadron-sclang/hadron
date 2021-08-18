#ifndef SRC_INCLUDE_HADRON_LIFETIME_HPP_
#define SRC_INCLUDE_HADRON_LIFETIME_HPP_

#include <list>
#include <set>

namespace hadron {

struct Interval {
    Interval() = delete;
    Interval(size_t f, size_t t, size_t v = 0): from(f), to(t), valueNumber(v) {}
    ~Interval() = default;

    size_t from;
    size_t to;
    size_t valueNumber;
};

// Ranges are currently [from, to) meaning usage is starting at from and up to, but not including, to.
struct Lifetime {
    Lifetime() = default;
    ~Lifetime() = default;

    // Adds an interval to the list, possibly merging with other intervals.
    void addInterval(size_t from, size_t to);

    std::list<Interval> intervals;
    std::set<size_t> usages;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_LIFETIME_HPP_