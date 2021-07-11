#include "hadron/VirtualJIT.hpp"

#include <limits>

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
    m_labels.emplace_back(m_instructions.size());
    m_instructions.emplace_back(Inst{Opcodes::kBgei, a, b});
    return m_labels.size() - 1;
}

JIT::Label VirtualJIT::jmpi() {
    m_labels.emplace_back(m_instructions.size());
    m_instructions.emplace_back(Inst{Opcodes::kJmpi});
    return m_labels.size() - 1;
}

void VirtualJIT::stxi(int offset, Reg address, Reg value) {
    m_instructions.emplace_back(Inst{Opcodes::kStxi, offset, address, value});
}

void VirtualJIT::prolog() {
    m_instructions.emplace_back(Inst{Opcodes::kProlog});
}

JIT::Label VirtualJIT::arg() {
    m_labels.emplace_back(m_instructions.size());
    m_instructions.emplace_back(Inst{Opcodes::kArg});
    return m_labels.size() - 1;
}

void VirtualJIT::getarg(Reg target, Label arg) {
    m_instructions.emplace_back(Inst{Opcodes::kGetarg, target, arg});
}

void VirtualJIT::allocai(int stackSizeBytes) {
    m_instructions.emplace_back(Inst{Opcodes::kAllocai, stackSizeBytes});
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

} // namespace hadron