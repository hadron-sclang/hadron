#include "hadron/OpcodeIterator.hpp"

namespace hadron {

void OpcodeIterator::setBuffer(uint8_t* address, size_t size) {
    m_startOfBytecode = address;
    m_currentBytecode = address;
    m_endOfBytecode = address + size;
}

bool OpcodeIterator::addByte(uint8_t byte) {
    if (m_endOfBytecode - m_currentBytecode < 1) { return false; }
    *m_currentBytecode = byte;
    ++m_currentBytecode;
    return true;
}

} // namespace hadron