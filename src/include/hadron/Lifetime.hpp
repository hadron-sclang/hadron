#ifndef SRC_INCLUDE_HADRON_LIFETIME_HPP_
#define SRC_INCLUDE_HADRON_LIFETIME_HPP_

#include <list>
#include <vector>

namespace hadron {

// Ranges are currently [from, to) meaning usage is starting at from and up to, but not including, to.

// Requirements for ranges data structure:
//  a) need to be able to compute next actual usage during the liveness range
//  b) merging of contiguous/overlapping ranges (or can do in post-process)
//  c) (optional) modify range start - yeah this makes things much harder, perhaps do algo trickery to avoid
//  d) "quick" query if a particular number is within the set of live ranges or no - but this is in linear order,
//      so could support a sort of "cursor" or some sort of caching of last query.
struct Lifetime {
    Lifetime() = default;
    ~Lifetime() = default;

    struct Interval {
        Interval() = delete;
        Interval(size_t f, size_t t): from(f), to(t) {}
        ~Interval() = default;

        size_t from;
        size_t to;
    };

    // Adds an interval to the list, possibly merging with other intervals.
    void addInterval(size_t from, size_t to);

    std::list<Interval> intervals;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_LIFETIME_HPP_