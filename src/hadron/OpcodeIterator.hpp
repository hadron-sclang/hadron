#ifndef SRC_HADRON_OPCODE_ITERATOR_HPP_
#define SRC_HADRON_OPCODE_ITERATOR_HPP_

#include "hadron/Arch.hpp"
#include "hadron/JIT.hpp"

#include <cstddef>
#include <cstdint>

namespace hadron {

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

class OpcodeIterator {
public:
    OpcodeIterator() = default;
    ~OpcodeIterator() = default;

    void setBuffer(uint8_t* address, size_t size);
    void reset();

    // All the serialization methods return a boolean if there was capacity to add the element or no.
    bool addAddr(JIT::Reg target, JIT::Reg a, JIT::Reg b);
    bool addAddi(JIT::Reg target, JIT::Reg a, Word b);
    bool addAndi(JIT::Reg target, JIT::Reg a, UWord b);
    bool addOri(JIT::Reg target, JIT::Reg a, UWord b);
    bool addXorr(JIT::Reg target, JIT::Reg a, JIT::Reg b);
    bool addMovr(JIT::Reg target, JIT::Reg value);
    bool addMovi(JIT::Reg target, Word value);
    // Returns the location of the branch address for later use with patchWord() or nullptr if in overflow.
    uint8_t* addBgei(JIT::Reg a, Word b);

    // |location| needs to be within the buffer. Overwrites whatever is there with |value|.
    bool patchWord(uint8_t* location, Word value);

    // Responses for the read commands are undefined if hasOverflow() is true.
    uint8_t readByte();


    uint8_t* getCurrent() const { return m_currentBytecode; }
    bool hasOverflow() const { return m_currentBytecode >= m_endOfBytecode; }
    // This can return values larger than |size| in the event of an overflow.
    size_t getSize() const { return m_currentBytecode - m_startOfBytecode; }

private:
    uint8_t reg(JIT::Reg r);
    bool addByte(uint8_t byte);
    bool addWord(Word word);
    bool addUWord(UWord word);
    bool addInt(int integer);

    uint8_t* m_startOfBytecode = nullptr;
    uint8_t* m_currentBytecode = nullptr;
    uint8_t* m_endOfBytecode = nullptr;
};

} // namespace hadron

#endif // SRC_HADRON_OPCODE_ITERATOR_HPP_