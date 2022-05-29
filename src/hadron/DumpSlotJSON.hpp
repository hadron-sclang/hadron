#ifndef SRC_HADRON_DUMP_SLOT_JSON_HPP_
#define SRC_HADRON_DUMP_SLOT_JSON_HPP_

#include "hadron/Slot.hpp"

#include <string>

namespace hadron {

struct ThreadContext;

// Produces a JSON string serialization of the provided |slot|.
std::string DumpSlotJSON(ThreadContext* context, Slot slot);

} // namespace hadron

#endif