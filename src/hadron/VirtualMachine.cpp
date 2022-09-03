#include "hadron/VirtualMachine.hpp"

#include "hadron/Arch.hpp"
#include "hadron/Slot.hpp"
#include "hadron/library/ArrayedCollection.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/OpcodeIterator.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"

#include "hadron/library/Schema.hpp"
#include "hadron/schema/Common/Collections/ArrayedCollectionSchema.hpp"
#include "spdlog/spdlog.h"

namespace {
// CPUs don't assume the signedness of the values in their registers until performing a signed or unsigned
// operation on the contents. To simulate this we need a portable way of recontextualizing the bits in a register
// as signed or unsigned.
template<typename A, typename B>
inline B signCast(A a) {
    B b;
    static_assert(sizeof(A) == sizeof(B));
    std::memcpy(&b, &a, sizeof(A));
    return b;
}
}

namespace hadron {

void VirtualMachine::executeMachineCode(ThreadContext* context, const int8_t* code) {
    // Clear the set arrays.
    std::memset(m_setGPRs.data(), 0, m_setGPRs.size());
    // The c ABI stack pointer has valid data in it, mark it as such
    writeGPR(JIT::Reg(0), 0);

    auto codeArray = resolveCodePointer(context, code);
    if (!codeArray) { return; }
    size_t size = codeArray.size() - (code - codeArray.start());

    auto iter = OpcodeReadIterator(code, size);
    while (!iter.hasOverflow()) {
        auto opcode = iter.peek();
        switch (opcode) {
        case kLoadCArgs2: {
            JIT::Reg regArg1, regArg2;
            if (!iter.loadCArgs2(regArg1, regArg2)) { return; }
            if (!writeGPR(regArg1, reinterpret_cast<UWord>(context))) { return; }
            if (!writeGPR(regArg2, reinterpret_cast<UWord>(code))) { return; }
        } break;

        case kAddr: {
            JIT::Reg regTarget, regA, regB;
            if (!iter.addr(regTarget, regA, regB)) { return; }
            UWord a, b;
            if (!readGPR(regA, a)) { return; }
            if (!readGPR(regB, b)) { return; }
            if (!writeGPR(regTarget, a + b)) { return; }
        } break;

        case kAddi: {
            JIT::Reg regTarget, regA;
            Word b;
            if (!iter.addi(regTarget, regA, b)) { return; }
            UWord a;
            if (!readGPR(regA, a)) { return; }
            if (!writeGPR(regTarget, signCast<Word, UWord>(signCast<Word, UWord>(a) + b))) { return; }
        } break;

        case kAndi: {
            JIT::Reg regTarget, regA;
            UWord b;
            if (!iter.andi(regTarget, regA, b)) { return; }
            UWord a;
            if (!readGPR(regA, a)) { return; }
            if (!writeGPR(regTarget, a & b)) { return; }
        } break;

        case kOri: {
            JIT::Reg regTarget, regA;
            UWord b;
            if (!iter.ori(regTarget, regA, b)) { return; }
            UWord a;
            if (!readGPR(regA, a)) { return; }
            if (!writeGPR(regTarget, a | b)) { return; }
        } break;

        case kXorr: {
            JIT::Reg regTarget, regA, regB;
            if (!iter.xorr(regTarget, regA, regB)) { return; }
            UWord a, b;
            if (!readGPR(regA, a)) { return; }
            if (!readGPR(regB, b)) { return; }
            if (!writeGPR(regTarget, a ^ b)) { return; }
        } break;

        case kMovr: {
            JIT::Reg regTarget, regValue;
            if (!iter.movr(regTarget, regValue)) { return; }
            UWord value;
            if (!readGPR(regValue, value)) { return; }
            if (!writeGPR(regTarget, value)) { return; }
        } break;

        case kMovi: {
            JIT::Reg regTarget;
            Word value;
            if (!iter.movi(regTarget, value)) { return; }
            if (!writeGPR(regTarget, signCast<Word, UWord>(value))) { return; }
        } break;

        case kMovAddr: {
            JIT::Reg regTarget;
            const int8_t* address;
            if (!iter.mov_addr(regTarget, address)) { return; }
            if (!writeGPR(regTarget, reinterpret_cast<UWord>(address))) { return; }
        } break;

        case kMoviU: {
            JIT::Reg regTarget;
            UWord value;
            if (!iter.movi_u(regTarget, value)) { return; }
            if (!writeGPR(regTarget, value)) { return; }
        } break;

        case kBgei: {
            JIT::Reg regA;
            Word b;
            const int8_t* address;
            if (!iter.bgei(regA, b, address)) { return; }
            UWord a;
            if (!readGPR(regA, a)) { return; }
            if (signCast<UWord, Word>(a) >= b) {
                codeArray = resolveCodePointer(context, address);
                if (!codeArray) { return; }
                iter.setBuffer(codeArray.start(), codeArray.size());
                iter.setCurrent(address);
            }
        } break;

        case kBeqi: {
            JIT::Reg regA;
            Word b;
            const int8_t* address;
            if (!iter.beqi(regA, b, address)) { return; }
            UWord a;
            if (!readGPR(regA, a)) { return; }
            if (signCast<UWord, Word>(a) == b) {
                codeArray = resolveCodePointer(context, address);
                if (!codeArray) { return; }
                iter.setBuffer(codeArray.start(), codeArray.size());
                iter.setCurrent(address);
            }
        } break;

        case kJmp: {
            const int8_t* address;
            if (!iter.jmp(address)) { return; }
            codeArray = resolveCodePointer(context, address);
            if (!codeArray) { return; }
            iter.setBuffer(codeArray.start(), codeArray.size());
            iter.setCurrent(address);
        } break;

        case kJmpr: {
            JIT::Reg r;
            if (!iter.jmpr(r)) { return; }
            UWord value;
            if (!readGPR(r, value)) { return; }
            if (!checkAddress(context, value)) { return; }
            int8_t* address = reinterpret_cast<int8_t*>(value);
            codeArray = resolveCodePointer(context, address);
            if (!codeArray) { return; }
            iter.setBuffer(codeArray.start(), codeArray.size());
            iter.setCurrent(address);
        } break;

        case kJmpi: {
            UWord location;
            if (!iter.jmpi(location)) { return; }
            if (!checkAddress(context, location)) { return; }
            int8_t* address = reinterpret_cast<int8_t*>(location);
            codeArray = resolveCodePointer(context, address);
            iter.setBuffer(codeArray.start(), codeArray.size());
            iter.setCurrent(address);
        } break;

        case kLdrL: {
            JIT::Reg regTarget, regAddress;
            if (!iter.ldr_l(regTarget, regAddress)) { return; }
            UWord address;
            if (!readGPR(regAddress, address)) { return; }
            if (regAddress != JIT::kContextPointerReg && !checkAddress(context, address)) { return; }
            uint64_t value = *(reinterpret_cast<uint64_t*>(address));
            if (!writeGPR(regTarget, static_cast<UWord>(value))) { return; }
        } break;

        case kLdiL: {
            JIT::Reg regTarget;
            void* address;
            if (!iter.ldi_l(regTarget, address)) { return; }
            uint64_t value = *(reinterpret_cast<uint64_t*>(address));
            if (!writeGPR(regTarget, static_cast<UWord>(value))) { return; }
        } break;

        case kLdxiW: {
            JIT::Reg regTarget, regAddress;
            int offset;
            if (!iter.ldxi_w(regTarget, regAddress, offset)) { return; }
            UWord address;
            if (!readGPR(regAddress, address)) { return; }
            if (regAddress != JIT::kContextPointerReg && !checkAddress(context, address)) { return; }
            UWord value = *(reinterpret_cast<UWord*>(reinterpret_cast<int8_t*>(address) + offset));
            if (!writeGPR(regTarget, value)) { return; }
        } break;

        case kLdxiI: {
            JIT::Reg regTarget, regAddress;
            int offset;
            if (!iter.ldxi_i(regTarget, regAddress, offset)) { return; }
            UWord address;
            if (!readGPR(regAddress, address)) { return; }
            if (regAddress != JIT::kContextPointerReg && !checkAddress(context, address)) { return; }
            uint32_t value = *(reinterpret_cast<uint32_t*>(reinterpret_cast<int8_t*>(address) + offset));
            if (!writeGPR(regTarget, static_cast<UWord>(value))) { return; }
        } break;

        case kLdxiL: {
            JIT::Reg regTarget, regAddress;
            int offset;
            if (!iter.ldxi_l(regTarget, regAddress, offset)) { return; }
            UWord address;
            if (!readGPR(regAddress, address)) { return; }
            if (regAddress != JIT::kContextPointerReg && !checkAddress(context, address)) { return; }
            uint64_t value = *(reinterpret_cast<uint64_t*>(reinterpret_cast<int8_t*>(address) + offset));
            if (!writeGPR(regTarget, static_cast<UWord>(value))) { return; }
        } break;

        case kStrI: {
            JIT::Reg regAddress, regValue;
            if (!iter.str_i(regAddress, regValue)) { return; }
            UWord address;
            if (!readGPR(regAddress, address)) { return; }
            if (regAddress != JIT::kContextPointerReg && !checkAddress(context, address)) { return; }
            UWord value;
            if (!readGPR(regValue, value)) { return; }
            *(reinterpret_cast<uint32_t*>(address)) = static_cast<uint32_t>(value);
        } break;

        case kStrL: {
            JIT::Reg regAddress, regValue;
            if (!iter.str_i(regAddress, regValue)) { return; }
            UWord address;
            if (!readGPR(regAddress, address)) { return; }
            if (regAddress != JIT::kContextPointerReg && !checkAddress(context, address)) { return; }
            UWord value;
            if (!readGPR(regValue, value)) { return; }
            *(reinterpret_cast<uint64_t*>(address)) = static_cast<uint64_t>(value);
        } break;

        case kStxiW: {
            int offset;
            JIT::Reg regAddress, regValue;
            if (!iter.stxi_w(offset, regAddress, regValue)) { return; }
            UWord address;
            if (!readGPR(regAddress, address)) { return; }
            if (regAddress != JIT::kContextPointerReg && !checkAddress(context, address)) { return; }
            UWord value;
            if (!readGPR(regValue, value)) { return; }
            *(reinterpret_cast<UWord*>(reinterpret_cast<int8_t*>(address) + offset)) = value;
        } break;

        case kStxiI: {
            int offset;
            JIT::Reg regAddress, regValue;
            if (!iter.stxi_w(offset, regAddress, regValue)) { return; }
            UWord address;
            if (!readGPR(regAddress, address)) { return; }
            if (regAddress != JIT::kContextPointerReg && !checkAddress(context, address)) { return; }
            UWord value;
            if (!readGPR(regValue, value)) { return; }
            *(reinterpret_cast<uint32_t*>(reinterpret_cast<int8_t*>(address) + offset)) = static_cast<uint32_t>(value);
        } break;

        case kStxiL: {
            int offset;
            JIT::Reg regAddress, regValue;
            if (!iter.stxi_w(offset, regAddress, regValue)) { return; }
            UWord address;
            if (!readGPR(regAddress, address)) { return; }
            if (regAddress != JIT::kContextPointerReg && !checkAddress(context, address)) { return; }
            UWord value;
            if (!readGPR(regValue, value)) { return; }
            *(reinterpret_cast<uint64_t*>(reinterpret_cast<int8_t*>(address) + offset)) = static_cast<uint64_t>(value);
        } break;

        case kRet:
            return;

        case kInvalid:
            SPDLOG_ERROR("Invalid Opcode");
            return;
        }
    }

    SPDLOG_ERROR("Hit end of buffer.");
}

bool VirtualMachine::readGPR(JIT::Reg reg, uint64_t& value) {
    int32_t r = reg + kNumberOfReservedRegisters;
    if (r < 0 || r >= static_cast<JIT::Reg>(kNumberOfPhysicalRegisters)) {
        SPDLOG_ERROR("reg read {} value out of range", reg);
        return false;
    }

    if (m_setGPRs[r] == 0) {
        SPDLOG_ERROR("reg {} read before written", reg);
        return false;
    }

    value = m_gprs[r];
    return true;
}

bool VirtualMachine::writeGPR(JIT::Reg reg, uint64_t value) {
    int32_t r = reg + kNumberOfReservedRegisters;

    if (r < 0 || r >= static_cast<JIT::Reg>(kNumberOfPhysicalRegisters)) {
        SPDLOG_ERROR("reg write {} value out of range", reg);
        return false;
    }

    m_setGPRs[r] = 1;
    m_gprs[r] = value;
    return true;
}

bool VirtualMachine::checkAddress(ThreadContext* context, UWord addr) {
    if (addr & Slot::kTagMask) {
        SPDLOG_ERROR("Pointer still tagged.");
        return false;
    }

    auto voidAddr = reinterpret_cast<void*>(addr);
    if (voidAddr == nullptr) {
        SPDLOG_ERROR("Pointer is null");
        return false;
    }

    library::Schema* containedObject = context->heap->getContainingObject(voidAddr);
    return containedObject != nullptr;
}

library::Int8Array VirtualMachine::resolveCodePointer(ThreadContext* context, const int8_t* addr) {
    if (reinterpret_cast<uintptr_t>(addr) & Slot::kTagMask) {
        SPDLOG_ERROR("code pointer still tagged.");
        return library::Int8Array();
    }

    library::Schema* objectPointer = context->heap->getContainingObject(addr);
    if (!objectPointer || objectPointer->_className != context->symbolTable->int8ArraySymbol().hash()) {
        SPDLOG_ERROR("attempt to resolve code pointer not pointing at code");
        return library::Int8Array();
    }

    return library::Int8Array(reinterpret_cast<schema::Int8ArraySchema*>(objectPointer));
}

} // namespace hadron