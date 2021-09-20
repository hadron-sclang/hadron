#include "hadron/VirtualJIT.hpp"

#include "hadron/ErrorReporter.hpp"

#include "fmt/format.h"

#include <iostream>
#include <limits>
#include <sstream>

namespace hadron {

VirtualJIT::VirtualJIT():
    JIT(std::make_shared<ErrorReporter>()),
    m_maxRegisters(64),
    m_maxFloatRegisters(64),
    m_addressCount(0) {}

VirtualJIT::VirtualJIT(std::shared_ptr<ErrorReporter> errorReporter):
    JIT(errorReporter),
    m_maxRegisters(std::numeric_limits<int32_t>::max()),
    m_maxFloatRegisters(std::numeric_limits<int32_t>::max()),
    m_addressCount(0) {}

VirtualJIT::VirtualJIT(std::shared_ptr<ErrorReporter> errorReporter, int maxRegisters, int maxFloatRegisters):
    JIT(errorReporter),
    m_maxRegisters(maxRegisters),
    m_maxFloatRegisters(maxFloatRegisters) { }

int VirtualJIT::getRegisterCount() const {
    if (m_maxRegisters < 3) {
        m_errorReporter->addInternalError("VirtualJIT instantiated with {} registers, requires a minimum of 3.");
    }
    return m_maxRegisters;
}

int VirtualJIT::getFloatRegisterCount() const {
    return m_maxFloatRegisters;
}

void VirtualJIT::addr(Reg target, Reg a, Reg b) {
    m_instructions.emplace_back(Inst{Opcodes::kAddr, target, a, b});
}

void VirtualJIT::addi(Reg target, Reg a, Word b) {
    m_instructions.emplace_back(Inst{Opcodes::kAddi, target, a, b});
}

void VirtualJIT::andi(Reg target, Reg a, UWord b) {
    Word uwordIndex = m_uwords.size();
    m_uwords.emplace_back(b);
    m_instructions.emplace_back(Inst{Opcodes::kAndi, target, a, uwordIndex});
}

void VirtualJIT::ori(Reg target, Reg a, UWord b) {
    Word uwordIndex = m_uwords.size();
    m_uwords.emplace_back(b);
    m_instructions.emplace_back(Inst{Opcodes::kOri, target, a, uwordIndex});
}

void VirtualJIT::xorr(Reg target, Reg a, Reg b) {
    m_instructions.emplace_back(Inst{Opcodes::kXorr, target, a, b});
}

void VirtualJIT::movr(Reg target, Reg value) {
    if (target != value) {
        m_instructions.emplace_back(Inst{Opcodes::kMovr, target, value});
    }
}

void VirtualJIT::movi(Reg target, Word value) {
    m_instructions.emplace_back(Inst{Opcodes::kMovi, target, value});
}

JIT::Label VirtualJIT::bgei(Reg a, Word b) {
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_instructions.size());
    m_instructions.emplace_back(Inst{Opcodes::kBgei, a, b, label});
    return label;
}

JIT::Label VirtualJIT::beqi(Reg a, Word b) {
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_instructions.size());
    m_instructions.emplace_back(Inst{Opcodes::kBeqi, a, b, label});
    return label;
}

JIT::Label VirtualJIT::jmp() {
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_instructions.size());
    m_instructions.emplace_back(Inst{Opcodes::kJmp, label});
    return label;
}

void VirtualJIT::jmpr(Reg r) {
    m_instructions.emplace_back(Inst{Opcodes::kJmpr, r});
}

void VirtualJIT::jmpi(Address location) {
    m_instructions.emplace_back(Inst{Opcodes::kJmpi, location});
}

void VirtualJIT::ldr_l(Reg target, Reg address) {
    m_instructions.emplace_back(Inst{Opcodes::kLdrL, target, address});
}

void VirtualJIT::ldxi_w(Reg target, Reg address, int offset) {
    m_instructions.emplace_back(Inst{Opcodes::kLdxiW, target, address, offset});
}

void VirtualJIT::ldxi_i(Reg target, Reg address, int offset) {
    m_instructions.emplace_back(Inst{Opcodes::kLdxiI, target, address, offset});
}

void VirtualJIT::ldxi_l(Reg target, Reg address, int offset) {
    m_instructions.emplace_back(Inst{Opcodes::kLdxiL, target, address, offset});
}

void VirtualJIT::str_i(Reg address, Reg value) {
    m_instructions.emplace_back(Inst{Opcodes::kStrI, address, value});
}

void VirtualJIT::str_l(Reg address, Reg value) {
    m_instructions.emplace_back(Inst{Opcodes::kStrL, address, value});
}

void VirtualJIT::stxi_w(int offset, Reg address, Reg value) {
    m_instructions.emplace_back(Inst{Opcodes::kStxiW, offset, address, value});
}

void VirtualJIT::stxi_i(int offset, Reg address, Reg value) {
    m_instructions.emplace_back(Inst{Opcodes::kStxiI, offset, address, value});
}

void VirtualJIT::stxi_l(int offset, Reg address, Reg value) {
    m_instructions.emplace_back(Inst{Opcodes::kStxiL, offset, address, value});
}

void VirtualJIT::ret() {
    m_instructions.emplace_back(Inst{Opcodes::kRet});
}

void VirtualJIT::retr(Reg r) {
    m_instructions.emplace_back(Inst{Opcodes::kRetr, r});
}

void VirtualJIT::reti(int value) {
    m_instructions.emplace_back(Inst{Opcodes::kReti, value});
}

JIT::Label VirtualJIT::label() {
    m_labels.emplace_back(m_instructions.size());
    m_instructions.emplace_back(Inst{Opcodes::kLabel});
    return m_labels.size() - 1;
}

JIT::Address VirtualJIT::address() {
    JIT::Address addressIndex = m_addressCount;
    ++m_addressCount;
    return addressIndex;
}

void VirtualJIT::patchHere(Label label) {
    if (label < static_cast<int>(m_labels.size())) {
        m_labels[label] = m_instructions.size();
        m_instructions.emplace_back(Inst{Opcodes::kPatchHere, label});
    } else {
        m_errorReporter->addInternalError(fmt::format("VirtualJIT patch label_{} contains out-of-bounds label argument "
            "as there are only {} labels", label, m_labels.size()));
    }
}

void VirtualJIT::patchThere(Label target, Address location) {
    if (target < static_cast<int>(m_labels.size()) && location < static_cast<int>(m_labels.size())) {
        m_labels[target] = m_labels[location];
        m_instructions.emplace_back(Inst{Opcodes::kPatchThere, target, location});
    } else {
        m_errorReporter->addInternalError(fmt::format("VirtualJIT patchat label_{} label_{} contains out-of-bounds "
            "label argument as there are only {} labels", target, location, m_labels.size()));
    }
}

} // namespace hadron