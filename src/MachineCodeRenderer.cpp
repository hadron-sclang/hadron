#include "hadron/MachineCodeRenderer.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/LightningJIT.hpp"
#include "hadron/VirtualJIT.hpp"

#include "fmt/format.h"

namespace hadron {

MachineCodeRenderer::MachineCodeRenderer(const VirtualJIT* virtualJIT, std::shared_ptr<ErrorReporter> errorReporter):
    m_virtualJIT(virtualJIT),
    m_machineJIT(std::make_unique<VirtualJIT>(errorReporter)),
    m_errorReporter(errorReporter),
    m_spillStackSize(0),
    m_spillStackOffsetBytes(0) {}

bool MachineCodeRenderer::render() {
    // Mark all machine registers as free.
    for (int i = 0; i < m_machineJIT->getRegisterCount(); ++i) {
        m_freeRegisters.emplace(i);
    }
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
            m_machineJIT->addr(inst[1], inst[2], inst[3]);
            break;

        case VirtualJIT::Opcodes::kAddi:
            m_machineJIT->addi(inst[1], inst[2], inst[3]);
            break;

        case VirtualJIT::Opcodes::kMovr:
            m_machineJIT->movr(inst[1], inst[2]);
            break;

        case VirtualJIT::Opcodes::kMovi:
            m_machineJIT->movi(inst[1], inst[2]);
            break;

        case VirtualJIT::Opcodes::kBgei:
            m_labels.emplace_back(m_machineJIT->bgei(inst[1], inst[2]));
            break;

        case VirtualJIT::Opcodes::kJmpi:
            m_labels.emplace_back(m_machineJIT->jmpi());
            break;

        case VirtualJIT::Opcodes::kStr:
            m_machineJIT->str(inst[1], inst[2]);
            break;

        case VirtualJIT::Opcodes::kSti:
            m_machineJIT->sti(m_virtualJIT->addresses()[inst[1]], inst[2]);
            break;

        case VirtualJIT::Opcodes::kStxi:
            m_machineJIT->stxi(inst[1], inst[2], inst[3]);
            break;

        case VirtualJIT::Opcodes::kProlog:
            m_machineJIT->prolog();
            break;

        case VirtualJIT::Opcodes::kArg:
            m_labels.emplace_back(m_machineJIT->arg());
            break;

        case VirtualJIT::Opcodes::kGetarg:
            m_machineJIT->getarg(inst[1], m_labels[inst[2]]);
            break;

        case VirtualJIT::Opcodes::kAllocai: {
            // Add room for register spilling on the stack here. Request room for one additional spot in the spill stack
            // to accomodate swaps at max capacity, to write out the value of the spilled register before unspilling
            // the incoming, previously spilled value.
            m_spillStackSize = std::max(0,
                1 + static_cast<int>(m_virtualJIT->registerUses().size() - m_machineJIT->getRegisterCount()));
            m_spillStackOffsetBytes = inst[1];
            for (int j = 0; j < m_spillStackSize; ++j) {
                m_freeSpillIndices.emplace(j);
            }
            m_machineJIT->allocai(m_spillStackOffsetBytes + (m_spillStackSize * sizeof(void*)));
        }   break;

        case VirtualJIT::Opcodes::kFrame:
            m_machineJIT->frame(m_spillStackOffsetBytes + (m_spillStackSize * sizeof(void*)));
            break;

        case VirtualJIT::Opcodes::kRet:
            m_machineJIT->ret();
            break;

        case VirtualJIT::Opcodes::kRetr:
            m_machineJIT->retr(inst[1]);
            break;

        case VirtualJIT::Opcodes::kReti:
            m_machineJIT->reti(inst[1]);
            break;

        case VirtualJIT::Opcodes::kEpilog:
            m_machineJIT->epilog();
            break;

        case VirtualJIT::Opcodes::kLabel:
            m_labels.emplace_back(m_machineJIT->label());
            break;

        case VirtualJIT::Opcodes::kPatchAt:
            m_machineJIT->patchAt(m_labels[inst[1]], m_labels[inst[2]]);
            break;

        case VirtualJIT::Opcodes::kPatch:
            m_machineJIT->patch(m_labels[inst[1]]);
            break;

        case VirtualJIT::Opcodes::kAlias:
            allocateRegister(inst[1]);
            break;

        case VirtualJIT::Opcodes::kUnalias:
            freeRegister(inst[1]);
            break;

        default:
            m_errorReporter->addInternalError(fmt::format("MachineCodeRenderer got unidentified VirtualJIT Opcode "
                "0x{:x}.", inst[0]));
            break;
        }
    }

    // Render final bytecode.
    return m_machineJIT->emit();
}

void MachineCodeRenderer::allocateRegister(VReg vReg) {
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
       reg = *(m_freeRegisters.begin());
        m_freeRegisters.erase(m_freeRegisters.begin());
    } else {
        reg = spill();
    }

    m_allocatedRegisters.emplace(std::make_pair(vReg, reg));
}

MachineCodeRenderer::MReg MachineCodeRenderer::mReg(VReg vReg) {
    auto allocIter = m_allocatedRegisters.find(vReg);
    if (allocIter != m_allocatedRegisters.end()) {
        return allocIter->second;
    }

    // Not allocated but active means it must be spilled, prepare to unspill.
    auto unspillIter = m_spilledRegisters.find(vReg);
    if (unspillIter == m_spilledRegisters.end()) {
        m_errorReporter->addInternalError(fmt::format("MachineCodeRenderer got request for machine register for virtual"
            " register %vr{}, which is neither allocated or spilled.", vReg));
        return 0;
    }

    MReg reg;
    // If there's no free regsiters we must first spill something.
    if (!m_freeRegisters.size()) {
        reg = spill();
    } else {
        reg = *(m_freeRegisters.begin());
        m_freeRegisters.erase(m_freeRegisters.end());
    }

    // Now we have a free register, unspill into it.
    m_machineJIT->ldxi(reg, JIT::kFramePointerReg, m_spillStackOffsetBytes + (unspillIter->second * sizeof(void*)));
    m_freeSpillIndices.emplace(unspillIter->second);
    m_spilledRegisters.erase(unspillIter);
    m_allocatedRegisters.emplace(std::make_pair(vReg, reg));

    return reg;
}

void MachineCodeRenderer::freeRegister(VReg vReg) {
    auto allocIter = m_allocatedRegisters.find(vReg);
    if (allocIter == m_allocatedRegisters.end()) {
        m_errorReporter->addInternalError(fmt::format("MachineCodeRenderer got request to free virtual register %vr{} "
            "that was not allocted.", vReg));
        return;
    }
    m_freeRegisters.emplace(allocIter->second);
    m_allocatedRegisters.erase(allocIter);
}

MachineCodeRenderer::MReg MachineCodeRenderer::spill() {
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
    // Spilling a register that isn't currently allocated is an error.
    if (spillIter == m_allocatedRegisters.end()) {
        m_errorReporter->addInternalError(fmt::format("MachineCodeRenderer got spill request and chose %vr{}, but "
            "%vr{} is not in the allocated registers list.", farthestUseRegister, farthestUseRegister));
        return 0;
    }

    MReg reg = spillIter->second;
    m_allocatedRegisters.erase(spillIter);

    // Having no room in spill storage is an error.
    if (!m_freeSpillIndices.size()) {
        m_errorReporter->addInternalError("MachineCodeRenderer got spill request but no room to spill.");
        return 0;
    }

    int spillIndex = *(m_freeSpillIndices.begin());
    m_freeSpillIndices.erase(m_freeSpillIndices.begin());
    m_machineJIT->stxi(m_spillStackOffsetBytes + (spillIndex * sizeof(void*)), JIT::kFramePointerReg, reg);
    m_spilledRegisters.emplace(std::make_pair(farthestUseRegister, spillIndex));
    return reg;
}


} // namespace hadron