#include "hadron/VirtualMachine.hpp"

#include "hadron/Arch.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/OpcodeIterator.hpp"
#include "hadron/ThreadContext.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

bool VirtualMachine::executeMachineCode(ThreadContext* context, const int8_t* code, int32_t size) {
    // We consider an attempt to jump to the callerIp with the frame pointer pointing at the caller frame as a valid
    // exit from machine code.
    auto callerFrame = library::Frame(context->framePointer->caller);
    auto callerIp = callerFrame.ip();

    // Clear the set arrays.
    std::memset(m_setGPRs.data(), 0, m_setGPRs.size());

    // Set up context, frame, and stack pointers.
    writeGPR(JIT::Reg(0), reinterpret_cast<uint64_t>(context));
    writeGPR(JIT::Reg(1), reinterpret_cast<uint64_t>(context->framePointer));
    writeGPR(JIT::Reg(2), reinterpret_cast<uint64_t>(context->stackPointer));

    auto iter = OpcodeReadIterator(code, size);
    while (!iter.hasOverflow()) {
        auto opcode = iter.peek();
        switch (opcode) {
        case kAddr: {
            JIT::Reg target, a, b;
            if (!iter.addr(target, a, b)) {
                SPDLOG_ERROR("Failed to decode addr");
                return false;
            }
            UWord regA, regB;
            if (!readGPR(a, regA)) { return false; }
            if (!readGPR(b, regB)) { return false; }
            if (!writeGPR(target, regA + regB)) { return false; }
        } break;

        case kAddi: {
            JIT::Reg target, a;
            Word b;
            if (!iter.addi(target, a, b)) {
                SPDLOG_ERROR("Failed to decode addi");
                return false;
            }
        } break;

        case kAndi:
        case kOri:
        case kXorr:
        case kMovr:
        case kMovi:
        case kMovAddr:
        case kMoviU:
        case kBgei:
        case kBeqi:
        case kJmp:
        case kJmpr:
        case kJmpi:
        case kLdrL:
        case kLdiL:
        case kLdxiW:
        case kLdxiI:
        case kLdxiL:
        case kStrI:
        case kStrL:
        case kStxiW:
        case kStxiI:
        case kStxiL:
        case kRet:
        case kRetr:
        case kReti:
        case kLabel:
        case kAddress:
        case kPatchHere:
        case kPatchThere:

        case kInvalid:
            SPDLOG_ERROR("Invalid Opcode");
            return false;
            break;
        }
    }

    SPDLOG_ERROR("Hit end of buffer.");
    return false;
}

bool VirtualMachine::readGPR(JIT::Reg reg, uint64_t& value) {
    if (reg < 0 || reg >= kNumberOfPhysicalRegisters) {
        SPDLOG_ERROR("reg read {} value out of range", reg);
        return false;
    }

    if (m_setGPRs[reg] == 0) {
        SPDLOG_ERROR("reg {} read before written", reg);
        return false;
    }

    value = m_gprs[reg];
    return true;
}

bool VirtualMachine::writeGPR(JIT::Reg reg, uint64_t value) {
    if (reg < 0 || reg >= kNumberOfPhysicalRegisters) {
        SPDLOG_ERROR("reg write {} value out of range", reg);
        return false;
    }

    m_setGPRs[reg] = 1;
    m_gprs[reg] = value;
    return true;
}

} // namespace hadron