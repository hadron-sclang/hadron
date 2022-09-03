#include "hadron/VirtualJIT.hpp"

#include "hadron/Arch.hpp"

#include "fmt/format.h"

#include <cassert>
#include <iostream>
#include <limits>
#include <sstream>

namespace hadron {

VirtualJIT::VirtualJIT():
    JIT(),
    m_maxRegisters(kNumberOfPhysicalRegisters),
    m_maxFloatRegisters(kNumberOfPhysicalFloatRegisters) {}

VirtualJIT::VirtualJIT(int maxRegisters, int maxFloatRegisters):
    JIT(),
    m_maxRegisters(maxRegisters),
    m_maxFloatRegisters(maxFloatRegisters) {
        assert(m_maxRegisters >= 3);
}

void VirtualJIT::begin(int8_t* buffer, size_t size) {
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

// We're never going to leave it! :)
size_t VirtualJIT::enterABI() {
    return 0;
}

void VirtualJIT::loadCArgs2(Reg arg1, Reg arg2) {
    m_iterator.loadCArgs2(arg1, arg2);
}

JIT::Reg VirtualJIT::getCStackPointerRegister() const {
    return JIT::Reg(0);
}

void VirtualJIT::leaveABI(size_t stackSize) {
    assert(stackSize == 0);
}

int VirtualJIT::getRegisterCount() const {
    return m_maxRegisters;
}

int VirtualJIT::getFloatRegisterCount() const {
    return m_maxFloatRegisters;
}

void VirtualJIT::addr(Reg target, Reg a, Reg b) {
    m_iterator.addr(target, a, b);
}

void VirtualJIT::addi(Reg target, Reg a, Word b) {
    m_iterator.addi(target, a, b);
}

void VirtualJIT::andi(Reg target, Reg a, UWord b) {
    m_iterator.andi(target, a, b);
}

void VirtualJIT::ori(Reg target, Reg a, UWord b) {
    m_iterator.ori(target, a, b);
}

void VirtualJIT::xorr(Reg target, Reg a, Reg b) {
    m_iterator.xorr(target, a, b);
}

void VirtualJIT::movr(Reg target, Reg value) {
    if (target != value) {
        m_iterator.movr(target, value);
    }
}

void VirtualJIT::movi(Reg target, Word value) {
    m_iterator.movi(target, value);
}

void VirtualJIT::movi_u(Reg target, UWord value) {
    m_iterator.movi_u(target, value);
}

JIT::Label VirtualJIT::mov_addr(Reg target) {
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_iterator.mov_addr(target));
    return label;
}

JIT::Label VirtualJIT::bgei(Reg a, Word b) {
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_iterator.bgei(a, b));
    return label;
}

JIT::Label VirtualJIT::beqi(Reg a, Word b) {
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_iterator.beqi(a, b));
    return label;
}

JIT::Label VirtualJIT::jmp() {
    JIT::Label label = m_labels.size();
    m_labels.emplace_back(m_iterator.jmp());
    return label;
}

void VirtualJIT::jmpr(Reg r) {
    m_iterator.jmpr(r);
}

void VirtualJIT::jmpi(Address location) {
    m_iterator.jmpi(location);
}

void VirtualJIT::ldr_l(Reg target, Reg address) {
    m_iterator.ldr_l(target, address);
}

void VirtualJIT::ldi_l(Reg target, void* address) {
    m_iterator.ldi_l(target, address);
}

void VirtualJIT::ldxi_w(Reg target, Reg address, int offset) {
    m_iterator.ldxi_w(target, address, offset);
}

void VirtualJIT::ldxi_i(Reg target, Reg address, int offset) {
    m_iterator.ldxi_i(target, address, offset);
}

void VirtualJIT::ldxi_l(Reg target, Reg address, int offset) {
    m_iterator.ldxi_l(target, address, offset);
}

void VirtualJIT::str_i(Reg address, Reg value) {
    m_iterator.str_i(address, value);
}

void VirtualJIT::str_l(Reg address, Reg value) {
    m_iterator.str_l(address, value);
}

void VirtualJIT::stxi_w(int offset, Reg address, Reg value) {
    m_iterator.stxi_w(offset, address, value);
}

void VirtualJIT::stxi_i(int offset, Reg address, Reg value) {
    m_iterator.stxi_i(offset, address, value);
}

void VirtualJIT::stxi_l(int offset, Reg address, Reg value) {
    m_iterator.stxi_l(offset, address, value);
}

void VirtualJIT::ret() {
    m_iterator.ret();
}

JIT::Address VirtualJIT::address() {
    JIT::Address a = m_addresses.size();
    m_addresses.emplace_back(m_iterator.current());
    return a;
}

void VirtualJIT::patchHere(Label label) {
    assert(label < static_cast<int>(m_labels.size()));
    m_iterator.patchWord(m_labels[label], reinterpret_cast<Word>(m_iterator.current()));
}

void VirtualJIT::patchThere(Label target, Address location) {
    assert(target < static_cast<Label>(m_labels.size()));
    assert(location < static_cast<Address>(m_addresses.size()));
    m_iterator.patchWord(m_labels[target], reinterpret_cast<Word>(m_addresses[location]));
}

const int8_t* VirtualJIT::getAddress(Address a) const {
    return m_addresses[a];
}

} // namespace hadron