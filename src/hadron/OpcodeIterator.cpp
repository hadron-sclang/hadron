#include "hadron/OpcodeIterator.hpp"

#include <cassert>

namespace hadron {

void OpcodeIterator::setBuffer(uint8_t* address, size_t size) {
    m_startOfBytecode = address;
    m_currentBytecode = address;
    m_endOfBytecode = address + size;
}

void OpcodeIterator::reset() {
    m_currentBytecode = m_startOfBytecode;
}

bool OpcodeIterator::addAddr(JIT::Reg target, JIT::Reg a, JIT::Reg b) {
    addByte(Opcodes::kAddr);
    addByte(reg(target));
    addByte(reg(a));
    addByte(reg(b));
    return !hasOverflow();
}

bool OpcodeIterator::addAddi(JIT::Reg target, JIT::Reg a, Word b) {
    addByte(Opcodes::kAddi);
    addByte(reg(target));
    addByte(reg(a));
    addWord(b);
    return !hasOverflow();
}

bool OpcodeIterator::addAndi(JIT::Reg target, JIT::Reg a, UWord b) {
    addByte(Opcodes::kAndi);
    addByte(reg(target));
    addByte(reg(a));
    addUWord(b);
    return !hasOverflow();
}

bool OpcodeIterator::addOri(JIT::Reg target, JIT::Reg a, UWord b) {
    addByte(Opcodes::kOri);
    addByte(reg(target));
    addByte(reg(a));
    addUWord(b);
    return !hasOverflow();
}

bool OpcodeIterator::addXorr(JIT::Reg target, JIT::Reg a, JIT::Reg b) {
    addByte(Opcodes::kXorr);
    addByte(reg(target));
    addByte(reg(a));
    addByte(reg(b));
    return !hasOverflow();
}

bool OpcodeIterator::addMovr(JIT::Reg target, JIT::Reg value) {
    addByte(Opcodes::kMovr);
    addByte(reg(target));
    addByte(reg(value));
    return !hasOverflow();
}

bool OpcodeIterator::addMovi(JIT::Reg target, Word value) {
    addByte(Opcodes::kMovi);
    addByte(reg(target));
    addWord(value);
    return !hasOverflow();
}

bool OpcodeIterator::patchWord(uint8_t* location, Word value) {
    if (location < m_startOfBytecode || location > m_endOfBytecode - sizeof(Word)) { return false; }
    for (size_t i = 0; i < sizeof(Word); ++i) {
        *location = (value >> i) & 0xff;
        ++location;
    }
    return true;
}

uint8_t OpcodeIterator::reg(JIT::Reg r) {
    assert(r >= -2 && r <= 253);
    return static_cast<uint8_t>(r);
}

bool OpcodeIterator::addByte(uint8_t byte) {
    bool overflow = true;
    if (m_currentBytecode < m_endOfBytecode) {
        *m_currentBytecode = byte;
        overflow = false;
    }
    ++m_currentBytecode;
    return overflow;
}

bool OpcodeIterator::addWord(Word word) {
    bool nonOverflow = true;
    for (size_t i = 0; i < sizeof(Word); ++i) {
        nonOverflow &= addByte((word >> i) & 0xff);
    }
    return nonOverflow;
}

bool OpcodeIterator::addUWord(UWord word) {
    bool nonOverflow = true;
    for (size_t i = 0; i < sizeof(UWord); ++i) {
        nonOverflow &= addByte((word >> i) & 0xff);
    }
    return nonOverflow;
}

bool OpcodeIterator::addInt(int integer) {
    bool nonOverflow = true;
    for (size_t i = 0; i < sizeof(int); ++i) {
        nonOverflow &= addByte((integer >> i) & 0xff);
    }
    return nonOverflow;
}

uint8_t OpcodeIterator::readByte() {
    if (hasOverflow()) return 0xff;
    uint8_t val = *m_currentBytecode;
    ++m_currentBytecode;
    return val;
}

} // namespace hadron