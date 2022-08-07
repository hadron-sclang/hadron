#ifndef SRC_HADRON_SLOT_DUMP_JSON_HPP_
#define SRC_HADRON_SLOT_DUMP_JSON_HPP_

#include "hadron/Slot.hpp"

#include <memory>
#include <string_view>

namespace hadron {

struct ThreadContext;

// To avoid copying strings around this class wraps the string and provides access to it via the json() accessor.
class SlotDumpJSON {
public:
    SlotDumpJSON();
    ~SlotDumpJSON();

    void dump(ThreadContext* context, Slot slot, bool prettyPrint);

    std::string_view json() const;

private:
    // pImpl pattern to protect including headers from contaminating json
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace hadron

#endif // SRC_HADRON_SLOT_DUMP_JSON_HPP_