#include "hadron/LinearFrame.hpp"

#include <cassert>

namespace hadron {

lir::VReg LinearFrame::hirToReg(library::HIRId hirId) {
    auto regIter = hirToRegMap.find(hirId.int32());
    if (regIter != hirToRegMap.end()) {
        return regIter->second;
    }

    assert(false);
    return lir::kInvalidVReg;
}

lir::VReg LinearFrame::append(library::HIRId hirId, std::unique_ptr<lir::LIR> lir) {
    if (lir->producesValue()) {
        lir->value = static_cast<lir::VReg>(vRegs.size());
    }

    if (hirId) {
        assert(lir->value != lir::kInvalidVReg);
        auto pair = hirToRegMap.emplace(std::make_pair(hirId.int32(), lir->value));
        assert(pair.second);
    }

    auto value = lir->value;
    instructions.emplace_back(std::move(lir));

    if (value != lir::kInvalidVReg) {
        assert(value == static_cast<lir::VReg>(vRegs.size()));
        auto iter = instructions.end();
        --iter;
        vRegs.emplace_back(iter);
    }

    return value;
}

} // namespace hadron