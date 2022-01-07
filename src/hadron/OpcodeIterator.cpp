#include "hadron/OpcodeIterator.hpp"

#include <cassert>

namespace hadron {

OpcodeWriteIterator::OpcodeWriteIterator(uint8_t* address, size_t size) {
    setBuffer(address, size);
}

void OpcodeWriteIterator::setBuffer(uint8_t* address, size_t size) {
    m_startOfBytecode = address;
    m_currentBytecode = address;
    m_endOfBytecode = address + size;
}

void OpcodeWriteIterator::reset() {
    m_currentBytecode = m_startOfBytecode;
}

bool OpcodeWriteIterator::addr(JIT::Reg target, JIT::Reg a, JIT::Reg b) {
    addByte(Opcode::kAddr);
    addByte(reg(target));
    addByte(reg(a));
    addByte(reg(b));
    return !hasOverflow();
}

bool OpcodeWriteIterator::addi(JIT::Reg target, JIT::Reg a, Word b) {
    addByte(Opcode::kAddi);
    addByte(reg(target));
    addByte(reg(a));
    addWord(b);
    return !hasOverflow();
}

bool OpcodeWriteIterator::andi(JIT::Reg target, JIT::Reg a, UWord b) {
    addByte(Opcode::kAndi);
    addByte(reg(target));
    addByte(reg(a));
    addUWord(b);
    return !hasOverflow();
}

bool OpcodeWriteIterator::ori(JIT::Reg target, JIT::Reg a, UWord b) {
    addByte(Opcode::kOri);
    addByte(reg(target));
    addByte(reg(a));
    addUWord(b);
    return !hasOverflow();
}

bool OpcodeWriteIterator::xorr(JIT::Reg target, JIT::Reg a, JIT::Reg b) {
    addByte(Opcode::kXorr);
    addByte(reg(target));
    addByte(reg(a));
    addByte(reg(b));
    return !hasOverflow();
}

bool OpcodeWriteIterator::movr(JIT::Reg target, JIT::Reg value) {
    addByte(Opcode::kMovr);
    addByte(reg(target));
    addByte(reg(value));
    return !hasOverflow();
}

bool OpcodeWriteIterator::movi(JIT::Reg target, Word value) {
    addByte(Opcode::kMovi);
    addByte(reg(target));
    addWord(value);
    return !hasOverflow();
}

uint8_t* OpcodeWriteIterator::bgei(JIT::Reg a, Word b) {
    addByte(Opcode::kBgei);
    addByte(reg(a));
    addWord(b);
    auto address = getCurrent();
    // Write an empty address into the bytecode, saving room for a patched address.
    addWord(0);
    if (hasOverflow()) { return nullptr; }
    return address;
}

uint8_t* OpcodeWriteIterator::beqi(JIT::Reg a, Word b) {
    addByte(Opcode::kBeqi);
    addByte(reg(a));
    addWord(b);
    auto address = getCurrent();
    addWord(0);
    if (hasOverflow()) { return nullptr; }
    return address;
}

uint8_t* OpcodeWriteIterator::jmp() {
    addByte(Opcode::kJmp);
    auto address = getCurrent();
    addWord(0);
    return address;
}

bool OpcodeWriteIterator::jmpr(JIT::Reg r) {
    addByte(Opcode::kJmpr);
    addByte(reg(r));
    return !hasOverflow();
}

bool OpcodeWriteIterator::jmpi(UWord location) {
    addByte(Opcode::kJmpi);
    addUWord(location);
    return !hasOverflow();
}

bool OpcodeWriteIterator::ldr_l(JIT::Reg target, JIT::Reg address) {
    addByte(Opcode::kLdrL);
    addByte(reg(target));
    addByte(reg(address));
    return !hasOverflow();
}

bool OpcodeWriteIterator::ldxi_w(JIT::Reg target, JIT::Reg address, int offset) {
    addByte(Opcode::kLdxiW);
    addByte(reg(target));
    addByte(reg(address));
    addInt(offset);
    return !hasOverflow();
}

bool OpcodeWriteIterator::ldxi_i(JIT::Reg target, JIT::Reg address, int offset) {
    addByte(Opcode::kLdxiI);
    addByte(reg(target));
    addByte(reg(address));
    addInt(offset);
    return !hasOverflow();
}

bool OpcodeWriteIterator::ldxi_l(JIT::Reg target, JIT::Reg address, int offset) {
    addByte(Opcode::kLdxiL);
    addByte(reg(target));
    addByte(reg(address));
    addInt(offset);
    return !hasOverflow();
}

bool OpcodeWriteIterator::str_i(JIT::Reg address, JIT::Reg value) {
    addByte(Opcode::kStrI);
    addByte(reg(address));
    addByte(reg(value));
    return !hasOverflow();
}

bool OpcodeWriteIterator::str_l(JIT::Reg address, JIT::Reg value) {
    addByte(Opcode::kStrL);
    addByte(reg(address));
    addByte(reg(value));
    return !hasOverflow();
}

bool OpcodeWriteIterator::stxi_w(int offset, JIT::Reg address, JIT::Reg value) {
    addByte(Opcode::kStxiW);
    addInt(offset);
    addByte(reg(address));
    addByte(reg(value));
    return !hasOverflow();
}

bool OpcodeWriteIterator::stxi_i(int offset, JIT::Reg address, JIT::Reg value) {
    addByte(Opcode::kStxiI);
    addInt(offset);
    addByte(reg(address));
    addByte(reg(value));
    return !hasOverflow();
}

bool OpcodeWriteIterator::stxi_l(int offset, JIT::Reg address, JIT::Reg value) {
    addByte(Opcode::kStxiL);
    addInt(offset);
    addByte(reg(address));
    addByte(reg(value));
    return !hasOverflow();
}

bool OpcodeWriteIterator::ret() {
    addByte(Opcode::kRet);
    return !hasOverflow();
}

bool OpcodeWriteIterator::retr(JIT::Reg r) {
    addByte(Opcode::kRetr);
    addByte(reg(r));
    return !hasOverflow();
}

bool OpcodeWriteIterator::reti(int value) {
    addByte(Opcode::kReti);
    addInt(value);
    return !hasOverflow();
}

bool OpcodeWriteIterator::patchWord(uint8_t* location, Word value) {
    if (location < m_startOfBytecode || location > m_endOfBytecode - sizeof(Word)) { return false; }
    for (size_t i = 0; i < sizeof(Word); ++i) {
        *location = (value >> i) & 0xff;
        ++location;
    }
    return true;
}

uint8_t OpcodeWriteIterator::reg(JIT::Reg r) {
    assert(r >= -2 && r <= 125);
    return static_cast<uint8_t>(r + 2);
}

bool OpcodeWriteIterator::addByte(uint8_t byte) {
    bool overflow = true;
    if (m_currentBytecode < m_endOfBytecode) {
        *m_currentBytecode = byte;
        overflow = false;
    }
    ++m_currentBytecode;
    return overflow;
}

bool OpcodeWriteIterator::addWord(Word word) {
    bool nonOverflow = true;
    for (size_t i = 0; i < sizeof(Word); ++i) {
        nonOverflow &= addByte((word >> (i * 8)) & 0xff);
    }
    return nonOverflow;
}

bool OpcodeWriteIterator::addUWord(UWord word) {
    bool nonOverflow = true;
    for (size_t i = 0; i < sizeof(UWord); ++i) {
        nonOverflow &= addByte((word >> (i * 8)) & 0xff);
    }
    return nonOverflow;
}

bool OpcodeWriteIterator::addInt(int integer) {
    bool nonOverflow = true;
    for (size_t i = 0; i < sizeof(int); ++i) {
        nonOverflow &= addByte((integer >> (i * 8)) & 0xff);
    }
    return nonOverflow;
}

OpcodeReadIterator::OpcodeReadIterator(const uint8_t* buffer, size_t size) {
    setBuffer(buffer, size);
}

void OpcodeReadIterator::setBuffer(const uint8_t* address, size_t size) {
    m_startOfBytecode = address;
    m_currentBytecode = address;
    m_endOfBytecode = address + size;
}

void OpcodeReadIterator::reset() {
    m_currentBytecode = m_startOfBytecode;
}

Opcode OpcodeReadIterator::peek() {
    if (hasOverflow()) { return Opcode::kInvalid; }
    return static_cast<Opcode>(*m_currentBytecode);
}

bool OpcodeReadIterator::addr(JIT::Reg& target, JIT::Reg& a, JIT::Reg& b) {
    assert(peek() == Opcode::kAddr);
    ++m_currentBytecode; // kAddr
    target = reg(readByte());
    a = reg(readByte());
    b = reg(readByte());
    return !hasOverflow();
}

bool OpcodeReadIterator::addi(JIT::Reg& target, JIT::Reg& a, Word& b) {
    assert(peek() == Opcode::kAddi);
    ++m_currentBytecode; // kAddi
    target = reg(readByte());
    a = reg(readByte());
    b = readWord();
    return !hasOverflow();
}

bool OpcodeReadIterator::andi(JIT::Reg& target, JIT::Reg& a, UWord& b) {
    assert(peek() == Opcode::kAndi);
    ++m_currentBytecode; // kAndi
    target = reg(readByte());
    a = reg(readByte());
    b = readUWord();
    return !hasOverflow();
}

bool OpcodeReadIterator::ori(JIT::Reg& target, JIT::Reg& a, UWord& b) {
    assert(peek() == Opcode::kOri);
    ++m_currentBytecode; // kOri
    target = reg(readByte());
    a = reg(readByte());
    b = readUWord();
    return !hasOverflow();
}

bool OpcodeReadIterator::xorr(JIT::Reg& target, JIT::Reg& a, JIT::Reg& b) {
    assert(peek() == Opcode::kXorr);
    ++m_currentBytecode; // kXorr
    target = reg(readByte());
    a = reg(readByte());
    b = reg(readByte());
    return !hasOverflow();
}

bool OpcodeReadIterator::movr(JIT::Reg& target, JIT::Reg& value) {
    assert(peek() == Opcode::kMovr);
    ++m_currentBytecode; // kMovr
    target = reg(readByte());
    value = reg(readByte());
    return !hasOverflow();
}

bool OpcodeReadIterator::movi(JIT::Reg& target, Word& value) {
    assert(peek() == Opcode::kMovi);
    ++m_currentBytecode; // kMovi
    target = reg(readByte());
    value = readWord();
    return !hasOverflow();
}

bool OpcodeReadIterator::bgei(JIT::Reg& a, Word& b, const uint8_t*& address) {
    assert(peek() == Opcode::kBgei);
    ++m_currentBytecode; // kBgei
    a = reg(readByte());
    b = readWord();
    address = reinterpret_cast<const uint8_t*>(readUWord());
    return !hasOverflow();
}

bool OpcodeReadIterator::beqi(JIT::Reg& a, Word& b, const uint8_t*& address) {
    assert(peek() == Opcode::kBeqi);
    ++m_currentBytecode; // kBeqi
    a = reg(readByte());
    b = readWord();
    address = reinterpret_cast<const uint8_t*>(readUWord());
    return !hasOverflow();
}

bool OpcodeReadIterator::jmp(const uint8_t*& address) {
    assert(peek() == Opcode::kJmp);
    ++m_currentBytecode; // kJmp
    address = reinterpret_cast<const uint8_t*>(readUWord());
    return !hasOverflow();
}

bool OpcodeReadIterator::jmpr(JIT::Reg& r) {
    assert(peek() == Opcode::kJmpr);
    ++m_currentBytecode; // kJmpr
    r = reg(readByte());
    return !hasOverflow();
}

bool OpcodeReadIterator::jmpi(UWord& location) {
    assert(peek() == Opcode::kJmpi);
    ++m_currentBytecode; // kJmpi
    location = readUWord();
    return !hasOverflow();
}

bool OpcodeReadIterator::ldr_l(JIT::Reg& target, JIT::Reg& address) {
    assert(peek() == Opcode::kLdrL);
    ++m_currentBytecode; // kLdrL
    target = reg(readByte());
    address = reg(readByte());
    return !hasOverflow();
}

bool OpcodeReadIterator::ldxi_w(JIT::Reg& target, JIT::Reg& address, int& offset) {
    assert(peek() == Opcode::kLdxiW);
    ++m_currentBytecode; // kLdxiW
    target = reg(readByte());
    address = reg(readByte());
    offset = readInt();
    return !hasOverflow();
}

bool OpcodeReadIterator::ldxi_i(JIT::Reg& target, JIT::Reg& address, int& offset) {
    assert(peek() == Opcode::kLdxiI);
    ++m_currentBytecode; // kLdxiI
    target = reg(readByte());
    address = reg(readByte());
    offset = readInt();
    return !hasOverflow();
}

bool OpcodeReadIterator::ldxi_l(JIT::Reg& target, JIT::Reg& address, int& offset) {
    assert(peek() == Opcode::kLdxiL);
    ++m_currentBytecode; // kLdxiL
    target = reg(readByte());
    address = reg(readByte());
    offset = readInt();
    return !hasOverflow();
}

bool OpcodeReadIterator::str_i(JIT::Reg& address, JIT::Reg& value) {
    assert(peek() == Opcode::kStrI);
    ++m_currentBytecode; // kStrI
    address = reg(readByte());
    value = reg(readByte());
    return !hasOverflow();
}

bool OpcodeReadIterator::str_l(JIT::Reg& address, JIT::Reg& value) {
    assert(peek() == Opcode::kStrL);
    ++m_currentBytecode; // kStrL
    address = reg(readByte());
    value = reg(readByte());
    return !hasOverflow();
}

bool OpcodeReadIterator::stxi_w(int& offset, JIT::Reg& address, JIT::Reg& value) {
    assert(peek() == Opcode::kStxiW);
    ++m_currentBytecode; // kStxiW
    offset = readInt();
    address = reg(readByte());
    value = reg(readByte());
    return !hasOverflow();
}

bool OpcodeReadIterator::stxi_i(int& offset, JIT::Reg& address, JIT::Reg& value) {
    assert(peek() == Opcode::kStxiI);
    ++m_currentBytecode; // kStxiI
    offset = readInt();
    address = reg(readByte());
    value = reg(readByte());
    return !hasOverflow();
}

bool OpcodeReadIterator::stxi_l(int& offset, JIT::Reg& address, JIT::Reg& value) {
    assert(peek() == Opcode::kStxiL);
    ++m_currentBytecode; // kStxiL
    offset = readInt();
    address = reg(readByte());
    value = reg(readByte());
    return !hasOverflow();
}

bool OpcodeReadIterator::ret() {
    assert(peek() == Opcode::kRet);
    ++m_currentBytecode; // kRet
    return !hasOverflow();
}

bool OpcodeReadIterator::retr(JIT::Reg& r) {
    assert(peek() == Opcode::kRetr);
    ++m_currentBytecode; // kRetr
    r = reg(readByte());
    return !hasOverflow();

}

bool OpcodeReadIterator::reti(int& value) {
    assert(peek() == Opcode::kReti);
    ++m_currentBytecode; // kReti
    value = readInt();
    return !hasOverflow();
}

JIT::Reg OpcodeReadIterator::reg(uint8_t r) {
    return static_cast<JIT::Reg>(r) - 2;
}

uint8_t OpcodeReadIterator::readByte() {
    uint8_t val = (m_currentBytecode >= m_endOfBytecode) ? 0 : *m_currentBytecode;
    ++m_currentBytecode;
    return val;
}

Word OpcodeReadIterator::readWord() {
    Word word = 0;
    for (size_t i = 0; i < sizeof(Word); ++i) {
        word = word >> 8;
        word = word | (static_cast<Word>(readByte()) << (8 * (sizeof(Word) - 1)));
    }
    return word;
}

UWord OpcodeReadIterator::readUWord() {
    UWord uword = 0;
    for (size_t i = 0; i < sizeof(UWord); ++i) {
        uword = uword >> 8;
        uword = uword | (static_cast<UWord>(readByte()) << (8 * (sizeof(UWord) - 1)));
    }
    return uword;
}

int OpcodeReadIterator::readInt() {
    int integer = 0;
    for (size_t i = 0; i < sizeof(int); ++i) {
        integer = integer >> 8;
        integer = integer | (static_cast<int>(readByte()) << (8 * (sizeof(int) - 1)));
    }
    return integer;
}

} // namespace hadron