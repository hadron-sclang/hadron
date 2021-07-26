#include "hadron/MachineCodeRenderer.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/Slot.hpp"
#include "hadron/VirtualJIT.hpp"

#include "fmt/format.h"

#include <algorithm>
#include <cassert>
#include <functional>

namespace hadron {

MachineCodeRenderer::MachineCodeRenderer(const VirtualJIT* virtualJIT, std::shared_ptr<ErrorReporter> errorReporter):
    m_virtualJIT(virtualJIT),
    m_errorReporter(errorReporter) { }

bool MachineCodeRenderer::render(JIT* jit) {
    int numberOfVirtualRegisters = static_cast<int>(m_virtualJIT->registerUses().size());
    // If JIT is a virtual JIT it may report a very large number of registers, this keeps it to the minimum of
    // the number of registers we have or (in the large number case) the number we need.
    int machineRegisterCount = std::min(jit->getRegisterCount(), numberOfVirtualRegisters);
    bool hasSpillArea = numberOfVirtualRegisters > machineRegisterCount;
    m_spillAreaSize = hasSpillArea ? numberOfVirtualRegisters : 0;

    // Mark all machine registers as free.
    for (int i = 0; i < machineRegisterCount; ++i) {
        m_freeRegisters.emplace_back(i);
    }
    // As this is an in-order list it should have heap property already.
    assert(std::is_heap(m_freeRegisters.begin(), m_freeRegisters.end(), std::less<int>()));

    // Set up the use cursors to all point at the first instruction.
    m_useCursors.resize(m_virtualJIT->registerUses().size(), 0);

    for (size_t i = 0; i < m_virtualJIT->instructions().size(); ++i) {
        if (!m_errorReporter->ok()) {
            return false;
        }

        // Advance use cursors past this instruction index or to the end of the use array.
        for (size_t j = 0; j < m_useCursors.size(); ++j) {
            while (m_useCursors[j] < m_virtualJIT->registerUses()[j].size() &&
                   m_virtualJIT->registerUses()[j][m_useCursors[j]] <= static_cast<int>(i)) {
                ++m_useCursors[j];
            }
        }

        const auto& inst = m_virtualJIT->instructions()[i];

        switch (inst[0]) {
        case VirtualJIT::Opcodes::kAddr:
            jit->addr(mReg(inst[1], jit), mReg(inst[2], jit), mReg(inst[3], jit));
            break;

        case VirtualJIT::Opcodes::kAddi:
            jit->addi(mReg(inst[1], jit), mReg(inst[2], jit), inst[3]);
            break;

        case VirtualJIT::Opcodes::kMovr:
            jit->movr(mReg(inst[1], jit), mReg(inst[2], jit));
            break;

        case VirtualJIT::Opcodes::kMovi:
            jit->movi(inst[1], inst[2]);
            break;

        case VirtualJIT::Opcodes::kBgei:
            m_labels.emplace_back(jit->bgei(inst[1], inst[2]));
            break;

        case VirtualJIT::Opcodes::kJmp:
            m_labels.emplace_back(jit->jmp());
            break;

        case VirtualJIT::Opcodes::kLdxiW:
            jit->ldxi_w(inst[1], inst[2], inst[3]);
            break;

        case VirtualJIT::Opcodes::kLdxiI:
            jit->ldxi_i(inst[1], inst[2], inst[3]);
            break;

        case VirtualJIT::Opcodes::kLdxiL:
            jit->ldxi_l(inst[1], inst[2], inst[3]);
            break;

        case VirtualJIT::Opcodes::kStrI:
            jit->str_i(inst[1], inst[2]);
            break;

        case VirtualJIT::Opcodes::kStxiW:
            jit->stxi_w(inst[1], inst[2], inst[3]);
            break;

        case VirtualJIT::Opcodes::kStxiI:
            jit->stxi_i(inst[1], inst[2], inst[3]);
            break;

        case VirtualJIT::Opcodes::kStxiL:
            jit->stxi_l(inst[1], inst[2], inst[3]);
            break;

        case VirtualJIT::Opcodes::kRet:
            jit->ret();
            break;

        case VirtualJIT::Opcodes::kRetr:
            jit->retr(inst[1]);
            break;

        case VirtualJIT::Opcodes::kReti:
            jit->reti(inst[1]);
            break;

        case VirtualJIT::Opcodes::kLabel:
            m_labels.emplace_back(jit->label());
            break;

        case VirtualJIT::Opcodes::kAddress:
            jit->address();
            break;

        case VirtualJIT::Opcodes::kPatchHere:
            jit->patchHere(m_labels[inst[1]]);
            break;

        case VirtualJIT::Opcodes::kPatchThere:
            jit->patchThere(m_labels[inst[1]], m_labels[inst[2]]);
            break;

        case VirtualJIT::Opcodes::kAlias:
            allocateRegister(inst[1], jit);
            break;

        case VirtualJIT::Opcodes::kUnalias:
            freeRegister(inst[1]);
            break;

        default:
            assert(false);
            return false;
        }
    }

    return true;
}

void MachineCodeRenderer::allocateRegister(VReg vReg, JIT* jit) {
    // Request to allocate a register already allocated is an error condition.
    if (m_allocatedRegisters.find(vReg) != m_allocatedRegisters.end()) {
        m_errorReporter->addInternalError(fmt::format("MachineCodeRenderer got request to allocate already allocated "
            "virtual register %vr{}.", vReg));
        return;
    }
    // Was this register spilled? This is also an error condition. Register spilling only should occur while allocated.
    if (m_spilledRegisters.find(vReg) != m_spilledRegisters.end()) {
        m_errorReporter->addInternalError(fmt::format("MachineCodeRenderer got request to allocate spilled virtual "
            "register %vr{}.", vReg));
        return;
    }

    MReg reg;
    // Allocate any unused registers first.
    if (m_freeRegisters.size()) {
        std::pop_heap(m_freeRegisters.begin(), m_freeRegisters.end(), std::less<int>());
        reg = m_freeRegisters.back();
        m_freeRegisters.pop_back();
    } else {
        reg = spill(jit);
    }

    m_allocatedRegisters.emplace(std::make_pair(vReg, reg));
}

MachineCodeRenderer::MReg MachineCodeRenderer::mReg(VReg vReg, JIT* jit) {
    if (vReg < 0) {
        return vReg;
    }

    auto allocIter = m_allocatedRegisters.find(vReg);
    if (allocIter != m_allocatedRegisters.end()) {
        return allocIter->second;
    }

    // Not allocated but active means it must be spilled, prepare to unspill.
    assert(m_spilledRegisters.count(vReg));

    MReg reg;
    // If there's no free regsiters we must first spill something.
    if (!m_freeRegisters.size()) {
        reg = spill(jit);
    } else {
        std::pop_heap(m_freeRegisters.begin(), m_freeRegisters.end(), std::less<int>());
        reg = m_freeRegisters.back();
        m_freeRegisters.pop_back();
    }

    // Now we have a free register, unspill into it.
    jit->ldxi_w(reg, JIT::kStackPointerReg, -((vReg * sizeof(Slot)) + offsetof(Slot, value.registerSpill)));
    m_spilledRegisters.erase(vReg);
    m_allocatedRegisters.emplace(std::make_pair(vReg, reg));

    return reg;
}

void MachineCodeRenderer::freeRegister(VReg vReg) {
    auto allocIter = m_allocatedRegisters.find(vReg);
    assert(allocIter != m_allocatedRegisters.end());
    m_freeRegisters.emplace_back(allocIter->second);
    std::push_heap(m_freeRegisters.begin(), m_freeRegisters.end(), std::less<int>());
    m_allocatedRegisters.erase(allocIter);
}

MachineCodeRenderer::MReg MachineCodeRenderer::spill(JIT* jit) {
    size_t farthestUseInst = m_virtualJIT->registerUses()[0][m_useCursors[0]];
    VReg farthestUseRegister = 0;
    for (size_t i = 1; i < m_useCursors.size(); ++i) {
        size_t nextUse = m_useCursors[i] < m_virtualJIT->registerUses()[i].size() ?
            m_virtualJIT->registerUses()[i][m_useCursors[i]] : m_virtualJIT->instructions().size();
        if (farthestUseInst < nextUse) {
            farthestUseInst = nextUse;
            farthestUseRegister = static_cast<VReg>(i);
        }
    }

    // Lookup, spill, then remove current machine register allocated to virtual register.
    auto spillIter = m_allocatedRegisters.find(farthestUseRegister);
    assert(spillIter != m_allocatedRegisters.end());
    MReg reg = spillIter->second;
    m_allocatedRegisters.erase(spillIter);

    jit->stxi_w(-((farthestUseRegister * sizeof(Slot)) + offsetof(Slot, value.registerSpill)),
        JIT::kStackPointerReg, reg);
    m_spilledRegisters.emplace(farthestUseRegister);
    return reg;
}


} // namespace hadron