#include "hadron/MachineCodeRenderer.hpp"

#include "hadron/LightningJIT.hpp"
#include "hadron/VirtualJIT.hpp"

namespace hadron {

MachineCodeRenderer::MachineCodeRenderer(const VirtualJIT* virtualJIT, std::shared_ptr<ErrorReporter> errorReporter):
    m_virtualJIT(virtualJIT),
    m_machineJIT(std::make_unique<LightningJIT>()),
    m_errorReporter(errorReporter) {}

bool MachineCodeRenderer::render() {
    for (const auto& inst : m_virtualJIT->instructions()) {
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
        case VirtualJIT::Opcodes::kJmpi:
        case VirtualJIT::Opcodes::kStr:
        case VirtualJIT::Opcodes::kSti:
        case VirtualJIT::Opcodes::kStxi:
        case VirtualJIT::Opcodes::kProlog:
        case VirtualJIT::Opcodes::kArg:
        case VirtualJIT::Opcodes::kGetarg:
        case VirtualJIT::Opcodes::kAllocai:
        case VirtualJIT::Opcodes::kRet:
        case VirtualJIT::Opcodes::kRetr:
        case VirtualJIT::Opcodes::kEpilog:
        case VirtualJIT::Opcodes::kLabel:
        case VirtualJIT::Opcodes::kPatchAt:
        case VirtualJIT::Opcodes::kPatch:
        case VirtualJIT::Opcodes::kAlias:
        case VirtualJIT::Opcodes::kUnalias:
            break;
        }
    }
    return true;
}


} // namespace hadron