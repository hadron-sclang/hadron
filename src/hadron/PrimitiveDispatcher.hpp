#ifndef SRC_HADRON_PRIMITIVE_DISPATCHER_HPP_
#define SRC_HADRON_PRIMITIVE_DISPATCHER_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Slot.hpp"

namespace hadron {

struct ThreadContext;

Slot dispatchPrimitive(ThreadContext* context, Hash primitiveName);

} // namespace hadron

#endif // SRC_HADRON_PRIMITIVE_DISPATCHER_HPP_