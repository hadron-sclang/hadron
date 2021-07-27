#include "hadron/VirtualJIT.hpp"

#include "hadron/ErrorReporter.hpp"

#include "fmt/format.h"

#include <iostream>
#include <limits>
#include <sstream>

namespace hadron {

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
    m_instructions.emplace_back(Inst{Opcodes::kAddr, use(target), use(a), use(b)});
}

void VirtualJIT::addi(Reg target, Reg a, int b) {
    m_instructions.emplace_back(Inst{Opcodes::kAddi, use(target), use(a), b});
}

void VirtualJIT::movr(Reg target, Reg value) {
    if (target != value) {
        m_instructions.emplace_back(Inst{Opcodes::kMovr, use(target), use(value)});
    }
}

void VirtualJIT::movi(Reg target, int value) {
    m_instructions.emplace_back(Inst{Opcodes::kMovi, use(target), value});
}

JIT::Label VirtualJIT::bgei(Reg a, int b) {
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_instructions.size());
    m_instructions.emplace_back(Inst{Opcodes::kBgei, use(a), b, label});
    return label;
}

JIT::Label VirtualJIT::jmp() {
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_instructions.size());
    m_instructions.emplace_back(Inst{Opcodes::kJmp, label});
    return label;
}

void VirtualJIT::jmpr(Reg r) {
    m_instructions.emplace_back(Inst{Opcodes::kJmpR, use(r)});
}

void VirtualJIT::ldxi_w(Reg target, Reg address, int offset) {
    m_instructions.emplace_back(Inst{Opcodes::kLdxiW, use(target), use(address), offset});
}

void VirtualJIT::ldxi_i(Reg target, Reg address, int offset) {
    m_instructions.emplace_back(Inst{Opcodes::kLdxiI, use(target), use(address), offset});
}

void VirtualJIT::ldxi_l(Reg target, Reg address, int offset) {
    m_instructions.emplace_back(Inst{Opcodes::kLdxiL, use(target), use(address), offset});
}

void VirtualJIT::str_i(Reg address, Reg value) {
    m_instructions.emplace_back(Inst{Opcodes::kStrI, use(address), use(value)});
}

void VirtualJIT::stxi_w(int offset, Reg address, Reg value) {
    m_instructions.emplace_back(Inst{Opcodes::kStxiW, offset, use(address), use(value)});
}

void VirtualJIT::stxi_i(int offset, Reg address, Reg value) {
    m_instructions.emplace_back(Inst{Opcodes::kStxiI, offset, use(address), use(value)});
}

void VirtualJIT::stxi_l(int offset, Reg address, Reg value) {
    m_instructions.emplace_back(Inst{Opcodes::kStxiL, offset, use(address), use(value)});
}

void VirtualJIT::ret() {
    m_instructions.emplace_back(Inst{Opcodes::kRet});
}

void VirtualJIT::retr(Reg r) {
    m_instructions.emplace_back(Inst{Opcodes::kRetr, use(r)});
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

void VirtualJIT::alias(Reg r) {
    if (r >= static_cast<int>(m_registerUses.size())) {
        m_registerUses.resize(r + 1);
    }
    m_instructions.emplace_back(Inst{Opcodes::kAlias, r});
}

void VirtualJIT::unalias(Reg r) {
    m_instructions.emplace_back(Inst{Opcodes::kUnalias, r});
}

bool VirtualJIT::toString(std::string& codeString) const {
    std::stringstream code;

    // Set width of label to width widest label string with padding.
    size_t labelWidth = fmt::format("label_{}:", m_labels.size()).size();

    for (size_t i = 0; i < m_instructions.size(); ++i) {
        const auto& inst = m_instructions[i];
        std::string label;
        for (size_t j = 0; j < m_labels.size(); ++j) {
            if (m_labels[j] == i) {
                if (label.empty()) {
                    label = fmt::format("label_{}:", j);
                } else {
                    code << fmt::format("label_{}:\n", j);
                }
            }
        }
        while (label.size() < labelWidth) {
            label += " ";
        }
        switch (inst[0]) {
        case kAddr:
            code << fmt::format("{} addr %vr{}, %vr{}, %vr{}\n", label, inst[1], inst[2], inst[3]);
            break;

        case kAddi:
            code << fmt::format("{} addi %vr{}, %vr{}, {}\n", label, inst[1], inst[2], inst[3]);
            break;

        case kMovr:
            code << fmt::format("{} movr %vr{}, %vr{}\n", label, inst[1], inst[2]);
             break;

        case kMovi:
            code << fmt::format("{} movi %vr{}, {}\n", label, inst[1], inst[2]);
            break;

        case kBgei:
            code << fmt::format("{} bgei %vr{}, {} label_{}\n", label, inst[1], inst[2], inst[3]);
            break;

        case kJmp:
            code << fmt::format("{} jmp label_{}\n", label, inst[1]);
            break;

        case kJmpR:
            code << fmt::format("{} jmpr %vr{}\n", label, inst[1]);
            break;

        case kLdxiW:
            code << fmt::format("{} ldxi_w %vr{}, %vr{}, 0x{:x}\n", label, inst[1], inst[2], inst[3]);
            break;

        case kLdxiI:
            code << fmt::format("{} ldxi_i %vr{}, %vr{}, 0x{:x}\n", label, inst[1], inst[2], inst[3]);
            break;

        case kStrI:
            code << fmt::format("{} str_i %vr{}, %vr{}\n", label, inst[1], inst[2]);
            break;

        case kStxiI:
            code << fmt::format("{} stxi_i 0x{:x}, %vr{}, %vr{}\n", label, inst[1], inst[2], inst[3]);
            break;

        case kRet:
            code << fmt::format("{} ret\n", label);
            break;

        case kRetr:
            code << fmt::format("{} retr %vr{}\n", label, inst[1]);
            break;

        case kReti:
            code << fmt::format("{} reti {}\n", label, inst[1]);
            break;

        case kLabel:
            code << fmt::format("{}\n", label);
            break;

        case kPatchHere:
            code << fmt::format("{} patch label_{}\n", label, inst[1]);
            break;

        case kPatchThere:
            code << fmt::format("{} patchat label_{}, label_{}\n", label, inst[1], inst[2]);
            break;

        case kAlias:
            code << fmt::format("{} alias %vr{}\n", label, inst[1]);
            break;

        case kUnalias:
            code << fmt::format("{} unalias %vr{}\n", label, inst[1]);
            break;

        default:
            m_errorReporter->addInternalError(fmt::format("VirtualJIT toString() encountered unknown opcode enum "
                "0x{:x}.", inst[0]));
            return false;
        }
    }

    code.flush();
    codeString = code.str();
    return true;
}

JIT::Reg VirtualJIT::use(JIT::Reg reg) {
    if (reg < 0) {
        return reg;
    }
    assert(reg < static_cast<int>(m_registerUses.size()));
    m_registerUses[reg].emplace_back(m_instructions.size());
    return reg;
}

} // namespace hadron