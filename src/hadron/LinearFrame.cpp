#include "hadron/LinearFrame.hpp"

#include <cassert>

namespace hadron {

lir::VReg LinearFrame::hirToReg(hir::ID hirId) {
    auto regIter = hirToRegMap.find(hirId);
    if (regIter != hirToRegMap.end()) {
        return regIter->second;
    }

    return lir::kInvalidVReg;
}

lir::VReg LinearFrame::append(hir::ID hirId, std::unique_ptr<lir::LIR> lir) {
    if (lir->producesValue()) {
        lir->value = static_cast<lir::VReg>(vRegs.size());
    }

    if (hirId != hir::kInvalidID) {
        assert(lir->value != lir::kInvalidVReg);
        auto pair = hirToRegMap.emplace(std::make_pair(hirId, lir->value));
        assert(pair.second);
    }

    auto value = lir->value;
    instructions.emplace_back(std::move(lir));

    if (value != lir::kInvalidVReg) {
        assert(value == static_cast<int32_t>(vRegs.size()));
        auto iter = instructions.end();
        --iter;
        vRegs.emplace_back(iter);
    }

    return value;
}

} // namespace hadron