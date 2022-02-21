#include "hadron/Hash.hpp"

#include "hadron/Signature.hpp"
#include "hadron/Slot.hpp"

#include "xxhash.h"

#include <cassert>
#include <vector>

namespace hadron {

Hash hash(std::string_view symbol) {
    return Slot::makeHash(XXH3_64bits(symbol.data(), symbol.size())).getHash();
}

Hash hash(const char* symbol, size_t length) {
    return Slot::makeHash(XXH3_64bits(symbol, length)).getHash();
}

Hash hashSignature(Signature* sig) {
    XXH64_state_t* state = XXH64_createState();

    // Seed the hash with the selector.
    XXH64_reset(state, sig->selector.hash());

    // Add the type flags for the arguments.
    XXH64_update(state, sig->argumentTypes.data(), sig->argumentTypes.size() * sizeof(TypeFlags));

    std::vector<Hash> classNames;
    assert(sig->argumentClassNames.size() == sig->argumentTypes.size());
    for (size_t i = 0; i < sig->argumentTypes.size(); ++i) {
        if (sig->argumentTypes[i] & TypeFlags::kObjectFlag && !sig->argumentClassNames[i].isNil()) {
            classNames.emplace_back(sig->argumentClassNames[i].hash());
        }
    }
    if (classNames.size()) {
        XXH64_update(state, classNames.data(), classNames.size() * sizeof(Hash));
    }

    Hash sigHash = XXH64_digest(state);
    XXH64_freeState(state);

    return Slot::makeHash(sigHash).getHash();
}


} // namespace hadron