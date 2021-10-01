#include "schema/Common/Core/Object.hpp"
#include "schema/Common/Collections/Collection.hpp"
#include "schema/Common/Collections/Set.hpp"
#include "schema/Common/Collections/Dictionary.hpp"

namespace hadron {

Slot IdentityDictionary::_IdentDict_At(ThreadContext* /* context */, Slot /* key */) {
    return Slot();
}

};