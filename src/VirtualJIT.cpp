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
    m_maxFloatRegisters(std::numeric_limits<int32_t>::max()) {}

VirtualJIT::VirtualJIT(std::shared_ptr<ErrorReporter> errorReporter, int maxRegisters, int maxFloatRegisters):
    JIT(errorReporter),
    m_maxRegisters(maxRegisters),
    m_maxFloatRegisters(maxFloatRegisters) { }

bool VirtualJIT::emit() {
    return true;
}

bool VirtualJIT::evaluate(Slot* /* value */) const {
    m_errorReporter->addInternalError("VirtualJIT got evaluation request.");
    return false;
}

void VirtualJIT::print() const {
    std::string code;
    if (toString(code)) {
        std::cout << code;
    }
}

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

JIT::Label VirtualJIT::jmpi() {
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_instructions.size());
    m_instructions.emplace_back(Inst{Opcodes::kJmpi, label});
    return label;
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

void VirtualJIT::sti_i(Address address, Reg value) {
    int addressNumber = m_addresses.size();
    m_addresses.emplace_back(address);
    m_instructions.emplace_back(Inst{Opcodes::kStiI, addressNumber, use(value)});
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

void VirtualJIT::prolog() {
    m_instructions.emplace_back(Inst{Opcodes::kProlog});
}

JIT::Label VirtualJIT::arg() {
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_instructions.size());
    m_instructions.emplace_back(Inst{Opcodes::kArg, label});
    return label;
}

void VirtualJIT::getarg_w(Reg target, Label arg) {
    m_instructions.emplace_back(Inst{Opcodes::kGetargW, use(target), arg});
}

void VirtualJIT::getarg_i(Reg target, Label arg) {
    m_instructions.emplace_back(Inst{Opcodes::kGetargI, use(target), arg});
}

void VirtualJIT::getarg_l(Reg target, Label arg) {
    m_instructions.emplace_back(Inst{Opcodes::kGetargL, use(target), arg});
}

void VirtualJIT::allocai(int stackSizeBytes) {
    m_instructions.emplace_back(Inst{Opcodes::kAllocai, stackSizeBytes});
}

void VirtualJIT::frame(int stackSizeBytes) {
    m_instructions.emplace_back(Inst{Opcodes::kFrame, stackSizeBytes});
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

void VirtualJIT::epilog() {
    m_instructions.emplace_back(Inst{Opcodes::kEpilog});
}

JIT::Label VirtualJIT::label() {
    m_labels.emplace_back(m_instructions.size());
    m_instructions.emplace_back(Inst{Opcodes::kLabel});
    return m_labels.size() - 1;
}

void VirtualJIT::patchAt(Label target, Label location) {
    if (target < static_cast<int>(m_labels.size()) && location < static_cast<int>(m_labels.size())) {
        m_labels[target] = m_labels[location];
        m_instructions.emplace_back(Inst{Opcodes::kPatchAt, target, location});
    } else {
        m_errorReporter->addInternalError(fmt::format("VirtualJIT patchat label_{} label_{} contains out-of-bounds "
            "label argument as there are only {} labels", target, location, m_labels.size()));
    }
}

void VirtualJIT::patch(Label label) {
    if (label < static_cast<int>(m_labels.size())) {
        m_labels[label] = m_instructions.size();
        m_instructions.emplace_back(Inst{Opcodes::kPatch, label});
    } else {
        m_errorReporter->addInternalError(fmt::format("VirtualJIT patch label_{} contains out-of-bounds label argument "
            "as there are only {} labels", label, m_labels.size()));
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

        case kJmpi:
            code << fmt::format("{} jmpi label_{}\n", label, inst[1]);
            break;

        case kLdxiI:
            code << fmt::format("{} ldxi_i %vr{}, %vr{}, 0x{:x}", label, inst[1], inst[2], inst[3]);
            break;

        case kStrI:
            code << fmt::format("{} str_i %vr{}, %vr{}\n", label, inst[1], inst[2]);
            break;

        case kStiI:
            code << fmt::format("{} sti_i 0x{:x}, %vr{}\n", label, m_addresses[inst[1]], inst[2]);
            break;

        case kStxiI:
            code << fmt::format("{} stxi_i 0x{:x}, %vr{}, %vr{}\n", label, inst[1], inst[2], inst[3]);
            break;

        case kProlog:
            code << fmt::format("{} prolog\n", label);
            break;

        case kArg:
            code << fmt::format("{} arg label_{}\n", label, inst[1]);
            break;

        case kGetargW:
            code << fmt::format("{} getarg_w %vr{}, label_{}\n", label, inst[1], inst[2]);
            break;

        case kGetargI:
            code << fmt::format("{} getarg_i %vr{}, label_{}\n", label, inst[1], inst[2]);
            break;

        case kGetargL:
            code << fmt::format("{} getarg_l %vr{}, label_{}\n", label, inst[1], inst[2]);
            break;

        case kAllocai:
            code << fmt::format("{} allocai {}\n", label, inst[1]);
            break;

        case kFrame:
            code << fmt::format("{} frame {}\n", label, inst[1]);
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

        case kEpilog:
            code << fmt::format("{} epilog\n", label);
            break;

        case kLabel:
            code << fmt::format("{}\n", label);
            break;

        case kPatchAt:
            code << fmt::format("{} patchat label_{}, label_{}\n", label, inst[1], inst[2]);
            break;

        case kPatch:
            code << fmt::format("{} patch label_{}\n", label, inst[1]);
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
    if (reg < static_cast<int>(m_registerUses.size())) {
        m_registerUses[reg].emplace_back(m_instructions.size());
    } else {
        m_errorReporter->addInternalError(fmt::format("VirtualJIT attempting to use unallocated register {}.", reg));
    }
    return reg;
}

} // namespace hadron