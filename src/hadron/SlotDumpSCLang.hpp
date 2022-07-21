#ifndef SRC_HADRON_SLOT_DUMP_SCLANG_HPP_
#define SRC_HADRON_SLOT_DUMP_SCLANG_HPP_

#include "hadron/library/Object.hpp"
#include "hadron/Slot.hpp"

#include <ostream>
#include <string>
#include <unordered_set>

namespace hadron {

struct ThreadContext;

// For interop with sclang, dumps the contents of a |slot| to the provided output stream in SCLang format, meaning
// a code block than when executed in sclang will reproduce the data structures and return the root.
class SlotDumpSCLang {
public:
    // Streams the contents of |slot| to |out|.
    static void dump(ThreadContext* context, Slot slot, std::ostream& out);

private:
    static std::string dumpValue(ThreadContext* context, Slot slot, std::ostream& out,
            std::unordered_set<Hash>& encodedObjects);
    static std::string dumpObject(ThreadContext* context, library::ObjectBase object, std::ostream& out,
            std::unordered_set<Hash>& encodedObjects);

};

} // namespace hadron


#endif