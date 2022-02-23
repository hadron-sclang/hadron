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
    bool validateFrame(const Frame* frame);
    bool validateLinearFrame(const LinearFrame* linearFrame, size_t numberOfBlocks);
    bool validateLifetimes(const LinearFrame* linearFrame);
    bool validateAllocation(const LinearFrame* linearFrame);
    bool validateResolution(const LinearFrame* linearFrame);
    bool validateEmission(const LinearFrame* linearFrame, library::Int8Array bytecodeArray);

private:
    bool validateSubScope(const Scope* scope, const Scope* parent, std::unordered_set<Block::ID>& blockIds,
            std::unordered_set<hir::NVID>& valueIds);
    bool validateSsaLir(const lir::LIR* lir, std::unordered_set<lir::VReg>& values);
    bool validateRegisterCoverage(const LinearFrame* linearFrame, size_t i, uint32_t vReg);
};

};

#endif // SRC_HADRON_VALIDATOR_HPP_