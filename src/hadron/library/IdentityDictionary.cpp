#include "schema/Common/Core/Object.hpp"
#include "schema/Common/Collections/Collection.hpp"
#include "schema/Common/Collections/Set.hpp"
#include "schema/Common/Collections/Dictionary.hpp"

namespace hadron {
namespace library {

// static
Slot IdentityDictionary::_IdentDict_At(ThreadContext* /* context */, Slot /* _this */, Slot /* key */) {
    return Slot();
}

} // namespace library
} // namespace hadron