#ifndef SRC_HADRON_VALIDATOR_HPP_
#define SRC_HADRON_VALIDATOR_HPP_

#include "hadron/Block.hpp"
#include "hadron/hir/HIR.hpp"
#include "hadron/library/ArrayedCollection.hpp"
#include "hadron/lir/LIR.hpp"

#include <memory>
#include <unordered_set>

namespace hadron {

struct Frame;
struct LinearFrame;
struct Scope;
struct ThreadContext;

// The validator can check the artifacts of each stage of code compilation for internal consistency.
class Validator {
public:
    // Checks for valid SSA form and that all members of Frame and contained Blocks are valid.
    static bool validateFrame(const Frame* frame);
    static bool validateLinearFrame(const LinearFrame* linearFrame, size_t numberOfBlocks);
    static bool validateLifetimes(const LinearFrame* linearFrame);
    static bool validateAllocation(const LinearFrame* linearFrame);
    static bool validateResolution(const LinearFrame* linearFrame);
    static bool validateEmission(const LinearFrame* linearFrame, library::Int8Array bytecodeArray);

private:
    static bool validateSubScope(const Scope* scope, const Scope* parent, std::unordered_set<Block::ID>& blockIds,
            std::unordered_set<hir::ID>& valueIds);
    static bool validateSsaLir(const lir::LIR* lir, std::unordered_set<lir::VReg>& values);
    static bool validateRegisterCoverage(const LinearFrame* linearFrame, size_t i, uint32_t vReg);
};

};

#endif // SRC_HADRON_VALIDATOR_HPP_