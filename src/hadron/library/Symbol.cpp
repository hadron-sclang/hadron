#include "schema/Common/Core/Object.hpp"
#include "schema/Common/Core/Symbol.hpp"

namespace hadron {

// Symbols are represented only by their hash, which packs into the Slot. The garbage collector maintains an
// IdentityDictionary as part of the root set, with the Symbol hashes as keys and the Strings as values.



} // namespace hadron