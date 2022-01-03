#include "hadron/VirtualJIT.hpp"

#include "fmt/format.h"

#include <cassert>
#include <iostream>
#include <limits>
#include <sstream>

namespace hadron {

VirtualJIT::VirtualJIT():
    JIT(),
    m_maxRegisters(64),
    m_maxFloatRegisters(64) {}


VirtualJIT::VirtualJIT(int maxRegisters, int maxFloatRegisters):
    JIT(),
    m_maxRegisters(maxRegisters),
    m_maxFloatRegisters(maxFloatRegisters) {
        assert(m_maxRegisters >= 3);
}

void VirtualJIT::begin(uint8_t* buffer, size_t size) {
    m_iterator.setBuffer(buffer, size);
    address();
}

bool VirtualJIT::hasJITBufferOverflow() {
    return m_iterator.hasOverflow();
}

void VirtualJIT::reset() {
    m_iterator.reset();
    m_labels.clear();
    m_addresses.clear();
    address();
}

JIT::Address VirtualJIT::end(size_t* sizeOut) {
    if (sizeOut) {
        *sizeOut = m_iterator.getSize();
    }
    // We always save the starting address as address 0.
    return 0;
}

int VirtualJIT::getRegisterCount() const {
    return m_maxRegisters;
}

int VirtualJIT::getFloatRegisterCount() const {
    return m_maxFloatRegisters;
}

void VirtualJIT::addr(Reg target, Reg a, Reg b) {
    m_iterator.addByte(Opcodes::kAddr);
    m_iterator.addByte(reg(target));
    m_iterator.addByte(reg(a));
    m_iterator.addByte(reg(b));
}

void VirtualJIT::addi(Reg target, Reg a, Word b) {
    m_iterator.addByte(Opcodes::kAddi);
    m_iterator.addByte(reg(target));
    m_iterator.addByte(reg(a));
    m_iterator.addWord(b);
}

void VirtualJIT::andi(Reg target, Reg a, UWord b) {
    m_iterator.addByte(Opcodes::kAndi);
    m_iterator.addByte(reg(target));
    m_iterator.addByte(reg(a));
    m_iterator.addUWord(b);
}

void VirtualJIT::ori(Reg target, Reg a, UWord b) {
    m_iterator.addByte(Opcodes::kOri);
    m_iterator.addByte(reg(target));
    m_iterator.addByte(reg(a));
    m_iterator.addUWord(b);
}

void VirtualJIT::xorr(Reg target, Reg a, Reg b) {
    m_iterator.addByte(Opcodes::kXorr);
    m_iterator.addByte(reg(target));
    m_iterator.addByte(reg(a));
    m_iterator.addByte(reg(b));
}

void VirtualJIT::movr(Reg target, Reg value) {
    if (target != value) {
        m_iterator.addByte(Opcodes::kMovr);
        m_iterator.addByte(reg(target));
        m_iterator.addByte(reg(value));
    }
}

void VirtualJIT::movi(Reg target, Word value) {
    m_iterator.addByte(Opcodes::kMovi);
    m_iterator.addByte(reg(target));
    m_iterator.addWord(value);
}

JIT::Label VirtualJIT::bgei(Reg a, Word b) {
    m_iterator.addByte(Opcodes::kBgei);
    m_iterator.addByte(reg(a));
    m_iterator.addWord(b);
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_iterator.getCurrent());
    // Write an empty address into the bytecode, saving room for a patched address.
    m_iterator.addWord(0);
    return label;
}

JIT::Label VirtualJIT::beqi(Reg a, Word b) {
    m_iterator.addByte(Opcodes::kBeqi);
    m_iterator.addByte(reg(a));
    m_iterator.addWord(b);
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_iterator.getCurrent());
    m_iterator.addWord(0);
    return label;
}

JIT::Label VirtualJIT::jmp() {
    m_iterator.addByte(Opcodes::kJmp);
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_iterator.getCurrent());
    m_iterator.addWord(0);
    return label;
}

void VirtualJIT::jmpr(Reg r) {
    m_iterator.addByte(Opcodes::kJmpr);
    m_iterator.addByte(reg(r));
}

void VirtualJIT::jmpi(Address location) {
    m_iterator.addByte(Opcodes::kJmpi);
    m_iterator.addUWord(location);
}

void VirtualJIT::ldr_l(Reg target, Reg address) {
    m_iterator.addByte(Opcodes::kLdrL);
    m_iterator.addByte(reg(target));
    m_iterator.addByte(reg(address));
}

void VirtualJIT::ldxi_w(Reg target, Reg address, int offset) {
    m_iterator.addByte(Opcodes::kLdxiW);
    m_iterator.addByte(reg(target));
    m_iterator.addByte(reg(address));
    m_iterator.addInt(offset);
}

void VirtualJIT::ldxi_i(Reg target, Reg address, int offset) {
    m_iterator.addByte(Opcodes::kLdxiI);
    m_iterator.addByte(reg(target));
    m_iterator.addByte(reg(address));
    m_iterator.addInt(offset);
}

void VirtualJIT::ldxi_l(Reg target, Reg address, int offset) {
    m_iterator.addByte(Opcodes::kLdxiL);
    m_iterator.addByte(reg(target));
    m_iterator.addByte(reg(address));
    m_iterator.addInt(offset);
}

void VirtualJIT::str_i(Reg address, Reg value) {
    m_iterator.addByte(Opcodes::kStrI);
    m_iterator.addByte(reg(address));
    m_iterator.addByte(reg(value));
}

void VirtualJIT::str_l(Reg address, Reg value) {
    m_iterator.addByte(Opcodes::kStrL);
    m_iterator.addByte(reg(address));
    m_iterator.addByte(reg(value));
}

void VirtualJIT::stxi_w(int offset, Reg address, Reg value) {
    m_iterator.addByte(Opcodes::kStxiW);
    m_iterator.addInt(offset);
    m_iterator.addByte(reg(address));
    m_iterator.addByte(reg(value));
}

void VirtualJIT::stxi_i(int offset, Reg address, Reg value) {
    m_iterator.addByte(Opcodes::kStxiI);
    m_iterator.addInt(offset);
    m_iterator.addByte(reg(address));
    m_iterator.addByte(reg(value));
}

void VirtualJIT::stxi_l(int offset, Reg address, Reg value) {
    m_iterator.addByte(Opcodes::kStxiL);
    m_iterator.addInt(offset);
    m_iterator.addByte(reg(address));
    m_iterator.addByte(reg(value));
}

void VirtualJIT::ret() {
    m_iterator.addByte(Opcodes::kRet);
}

void VirtualJIT::retr(Reg r) {
    m_iterator.addByte(Opcodes::kRetr);
    m_iterator.addByte(reg(r));
}

void VirtualJIT::reti(int value) {
    m_iterator.addByte(Opcodes::kReti);
    m_iterator.addInt(value);
}

JIT::Label VirtualJIT::label() {
    m_iterator.addByte(Opcodes::kLabel);
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_iterator.getCurrent());
    m_iterator.addWord(0);
    return label;
}

JIT::Address VirtualJIT::address() {
    JIT::Address a = m_addresses.size();
    m_addresses.emplace_back(m_iterator.getCurrent());
    return a;
}

void VirtualJIT::patchHere(Label label) {
    assert(label < static_cast<int>(m_labels.size()));
    m_iterator.patchWord(m_labels[label], reinterpret_cast<Word>(m_iterator.getCurrent()));
}

void VirtualJIT::patchThere(Label target, Address location) {
    assert(target < static_cast<Label>(m_labels.size()));
    assert(location < static_cast<Address>(m_addresses.size()));
    m_iterator.patchWord(m_labels[target], reinterpret_cast<Word>(m_addresses[location]));
}

uint8_t VirtualJIT::reg(Reg r) {
    assert(r >= -2 && r <= 253);
    return static_cast<uint8_t>(r);
}

} // namespace hadron