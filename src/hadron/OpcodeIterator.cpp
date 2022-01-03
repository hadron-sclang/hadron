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

bool OpcodeIterator::patchWord(uint8_t* location, Word value) {
    if (location < m_startOfBytecode || location > m_endOfBytecode - sizeof(Word)) { return false; }
    for (size_t i = 0; i < sizeof(Word); ++i) {
        *location = (value >> i) & 0xff;
        ++location;
    }
    return true;
}

} // namespace hadron