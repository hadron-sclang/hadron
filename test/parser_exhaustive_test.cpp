/*! Parser Exhaustive Test
 *
 * This program uses a version of the legacy SuperCollider grammar, as extracted from the SC Bison source file, to
 * generate one (or more) of every possible gramatically valid sclang source string. It can then process the test
 * strings in a few different ways:
 *
 * Validation: The sclang source strings are parsed with the Hadron parser, and the resulting parse tree is checked
 *      against the expected tree as determined by the grammar that generated the string. The code can be instrumented
 *      to collect information about possible memory leaks/corruption and other stability analysis tools.
 * Benchmarking: TODO - although parsing is typically only a small part of the overall time spent by a compiler it is
 *      still interesting to collect benchmarking statistics on parsing performance, for comparison against the legacy
 *      SC parser, as well as for performance regression comparison.
 */
#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include "GrammarIterator.hpp"
#include "Parser.hpp"

#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

} // namespace

// 18358659134393842662 is the current count of all valid patterns as the outer product of all possible expansions
// of each term of the grammar. Even processing a billion patterns per second this program will take many thousands of
// years to run to completion. I think pursuing this still has value, although next steps for investigation involve
// removing some of the redundancy from the tree, and distributing the work across many cores (and likely many
// computers).

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    hadron::GrammarIterator grammarIterator;
    if (!grammarIterator.buildGrammarTree()) {
        spdlog::error("Failed to build grammar iterator tree.");
        return -1;
    }

    spdlog::info("Counted {} possible expansions.", grammarIterator.countExpansions());

    return 0;
}
