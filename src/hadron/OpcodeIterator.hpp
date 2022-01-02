#ifndef SRC_HADRON_OPCODE_ITERATOR_HPP_
#define SRC_HADRON_OPCODE_ITERATOR_HPP_

#include <cstddef>
#include <cstdint>

enum Opcodes : uint8_t {
    kAddr       = 0x01,
    kAddi       = 0x02,
    kAndi       = 0x03,
    kOri        = 0x04,
    kXorr       = 0x05,
    kMovr       = 0x06,
    kMovi       = 0x07,
    kBgei       = 0x08,
    kBeqi       = 0x09,
    kJmp        = 0x0a,
    kJmpr       = 0x0b,
    kJmpi       = 0x0c,
    kLdrL       = 0x0d,
    kLdxiW      = 0x0e,
    kLdxiI      = 0x0f,
    kLdxiL      = 0x10,
    kStrI       = 0x11,
    kStrL       = 0x12,
    kStxiW      = 0x13,
    kStxiI      = 0x14,
    kStxiL      = 0x15,
    kRet        = 0x16,
    kRetr       = 0x17,
    kReti       = 0x18,
    kLabel      = 0x19,
    kAddress    = 0x1a,
    kPatchHere  = 0x1b,
    kPatchThere = 0x1c,
};

namespace hadron {

class OpcodeIterator {
public:
    OpcodeIterator() = default;
    ~OpcodeIterator() = default;

    void setBuffer(uint8_t* address, size_t size);

    // All the serialization methods return a boolean if there was capacity to add the element or no.
    bool addByte(uint8_t byte);

private:
    uint8_t* m_startOfBytecode = nullptr;
    uint8_t* m_currentBytecode = nullptr;
    uint8_t* m_endOfBytecode = nullptr;
};

} // namespace hadron

#endif // SRC_HADRON_OPCODE_ITERATOR_HPP_