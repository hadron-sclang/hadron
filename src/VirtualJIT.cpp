#include "hadron/VirtualJIT.hpp"

#include "fmt/format.h"

#include <limits>
#include <sstream>

namespace hadron {

bool VirtualJIT::emit() {
    return true;
}

Slot VirtualJIT::value() {
    return Slot();
}

int VirtualJIT::getRegisterCount() const {
    return std::numeric_limits<int32_t>::max();
}

int VirtualJIT::getFloatRegisterCount() const {
    return std::numeric_limits<int32_t>::max();
}

void VirtualJIT::addr(Reg target, Reg a, Reg b) {
    m_instructions.emplace_back(Inst{Opcodes::kAddr, target, a, b});
}

void VirtualJIT::addi(Reg target, Reg a, int b) {
    m_instructions.emplace_back(Inst{Opcodes::kAddi, target, a, b});
}

void VirtualJIT::movr(Reg target, Reg value) {
    m_instructions.emplace_back(Inst{Opcodes::kMovr, target, value});
}

void VirtualJIT::movi(Reg target, int value) {
    m_instructions.emplace_back(Inst{Opcodes::kMovi, target, value});
}

JIT::Label VirtualJIT::bgei(Reg a, int b) {
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_instructions.size());
    m_instructions.emplace_back(Inst{Opcodes::kBgei, a, b, label});
    return label;
}

JIT::Label VirtualJIT::jmpi() {
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_instructions.size());
    m_instructions.emplace_back(Inst{Opcodes::kJmpi, label});
    return label;
}

void VirtualJIT::str(Reg address, Reg value) {
    m_instructions.emplace_back(Inst{Opcodes::kStr, address, value});
}

void VirtualJIT::sti(Address address, Reg value) {
    int addressNumber = m_addresses.size();
    m_addresses.emplace_back(address);
    m_instructions.emplace_back(Inst{Opcodes::kSti, addressNumber, value});
}

void VirtualJIT::stxi(int offset, Reg address, Reg value) {
    m_instructions.emplace_back(Inst{Opcodes::kStxi, offset, address, value});
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

void VirtualJIT::getarg(Reg target, Label arg) {
    m_instructions.emplace_back(Inst{Opcodes::kGetarg, target, arg});
}

void VirtualJIT::allocai(int stackSizeBytes) {
    m_instructions.emplace_back(Inst{Opcodes::kAllocai, stackSizeBytes});
}

void VirtualJIT::ret() {
    m_instructions.emplace_back(Inst{Opcodes::kRet});
}

void VirtualJIT::retr(Reg r) {
    m_instructions.emplace_back(Inst{Opcodes::kRetr, r});
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
    m_instructions.emplace_back(Inst{Opcodes::kPatchAt, target, location});
    m_labels[target] = location;
}

void VirtualJIT::patch(Label label) {
    m_instructions.emplace_back(Inst{Opcodes::kPatch, label});
    m_labels[label] = m_instructions.size();
}

void VirtualJIT::alias(Reg r) {
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
            code << fmt::format("{} addr %r{}, %r{}, %r{}\n", label, inst[1], inst[2], inst[3]);
            break;

        case kAddi:
            code << fmt::format("{} addi %r{}, %r{}, {}\n", label, inst[1], inst[2], inst[3]);
            break;

        case kMovr:
            code << fmt::format("{} movr %r{}, %r{}\n", label, inst[1], inst[2]);
            break;

        case kMovi:
            code << fmt::format("{} movi %r{}, {}\n", label, inst[1], inst[2]);
            break;

        case kBgei:
            code << fmt::format("{} bgei %r{}, {} label_{}\n", label, inst[1], inst[2], inst[3]);
            break;

        case kJmpi:
            code << fmt::format("{} jmpi label_{}\n", label, inst[1]);
            break;

        case kStr:
            code << fmt::format("{} str %r{}, %r{}\n", label, inst[1], inst[2]);
            break;

        case kSti:
            code << fmt::format("{} sti 0x{:016x}, %r{}\n", label, m_addresses[inst[1]], inst[2]);
            break;

        case kStxi:
            code << fmt::format("{} stxi 0x{:08x}, %r{}, %r{}\n", label, inst[1], inst[2], inst[3]);
            break;

        case kProlog:
            code << fmt::format("{} prolog\n", label);
            break;

        case kArg:
            code << fmt::format("{} arg label_{}\n", label, inst[1]);
            break;

        case kGetarg:
            code << fmt::format("{} getarg %r{}, label_{}\n", label, inst[1], inst[2]);
            break;

        case kAllocai:
            code << fmt::format("{} allocai {}\n", label, inst[1]);
            break;

        case kRet:
            code << fmt::format("{} ret\n", label);
            break;

        case kRetr:
            code << fmt::format("{} retr %r{}\n", label, inst[1]);
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
            code << fmt::format("{} alias %r{}\n", label, inst[1]);
            break;

        case kUnalias:
            code << fmt::format("{} unalias %r{}\n", label, inst[1]);
            break;

        default:
            return false;
        }
    }

    code.flush();
    codeString = code.str();
    return true;
}

} // namespace hadron