#include "hadron/LinearFrame.hpp"

#include <cassert>

namespace hadron {

void LinearFrame::append(hir::ID hirId, std::unique_ptr<lir::LIR> lir) {
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
}

} // namespace hadron