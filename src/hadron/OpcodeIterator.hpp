#ifndef SRC_HADRON_OPCODE_ITERATOR_HPP_
#define SRC_HADRON_OPCODE_ITERATOR_HPP_

#include "hadron/Arch.hpp"
#include "hadron/JIT.hpp"

#include <cstddef>
#include <cstdint>

namespace hadron {

enum Opcode : uint8_t {
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

    kInvalid    = 0xff
};

class OpcodeWriteIterator {
public:
    OpcodeWriteIterator() = default;
    OpcodeWriteIterator(uint8_t* address, size_t size);
    ~OpcodeWriteIterator() = default;

    void setBuffer(uint8_t* address, size_t size);
    void reset();

    // All the serialization methods return a boolean if there was capacity to add the element or no.
    bool addr(JIT::Reg target, JIT::Reg a, JIT::Reg b);
    bool addi(JIT::Reg target, JIT::Reg a, Word b);
    bool andi(JIT::Reg target, JIT::Reg a, UWord b);
    bool ori(JIT::Reg target, JIT::Reg a, UWord b);
    bool xorr(JIT::Reg target, JIT::Reg a, JIT::Reg b);
    bool movr(JIT::Reg target, JIT::Reg value);
    bool movi(JIT::Reg target, Word value);
    // Returns the location of the branch address for later use with patchWord() or nullptr if in overflow.
    uint8_t* bgei(JIT::Reg a, Word b);
    uint8_t* beqi(JIT::Reg a, Word b);
    uint8_t* jmp();
    bool jmpr(JIT::Reg r);
    bool jmpi(UWord location);
    bool ldr_l(JIT::Reg target, JIT::Reg address);
    bool ldxi_w(JIT::Reg target, JIT::Reg address, int offset);
    bool ldxi_i(JIT::Reg target, JIT::Reg address, int offset);
    bool ldxi_l(JIT::Reg target, JIT::Reg address, int offset);
    bool str_i(JIT::Reg address, JIT::Reg value);
    bool str_l(JIT::Reg address, JIT::Reg value);
    bool stxi_w(int offset, JIT::Reg address, JIT::Reg value);
    bool stxi_i(int offset, JIT::Reg address, JIT::Reg value);
    bool stxi_l(int offset, JIT::Reg address, JIT::Reg value);
    bool ret();
    bool retr(JIT::Reg r);
    bool reti(int value);
    // |location| needs to be within the buffer. Overwrites whatever is there with |value|.
    bool patchWord(uint8_t* location, Word value);

    uint8_t* getCurrent() const { return m_currentBytecode; }
    bool hasOverflow() const { return m_currentBytecode > m_endOfBytecode; }
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

class OpcodeReadIterator {
public:
    OpcodeReadIterator() = default;
    OpcodeReadIterator(const uint8_t* buffer, size_t size);
    ~OpcodeReadIterator() = default;

    void setBuffer(const uint8_t* address, size_t size);
    void reset();

    // Returns opcode at current() or kInvalid if outside of buffer.
    Opcode peek();
    bool addr(JIT::Reg& target, JIT::Reg& a, JIT::Reg& b);
    bool addi(JIT::Reg& target, JIT::Reg& a, Word& b);
    bool andi(JIT::Reg& target, JIT::Reg& a, UWord& b);
    bool ori(JIT::Reg& target, JIT::Reg& a, UWord& b);
    bool xorr(JIT::Reg& target, JIT::Reg& a, JIT::Reg& b);
    bool movr(JIT::Reg& target, JIT::Reg& value);
    bool movi(JIT::Reg& target, Word& value);
    bool bgei(JIT::Reg& a, Word& b, const uint8_t*& address);
    bool beqi(JIT::Reg& a, Word& b, const uint8_t*& address);
    bool jmp(const uint8_t*& address);
    bool jmpr(JIT::Reg& r);
    bool jmpi(UWord& location);
    bool ldr_l(JIT::Reg& target, JIT::Reg& address);
    bool ldxi_w(JIT::Reg& target, JIT::Reg& address, int& offset);
    bool ldxi_i(JIT::Reg& target, JIT::Reg& address, int& offset);
    bool ldxi_l(JIT::Reg& target, JIT::Reg& address, int& offset);
    bool str_i(JIT::Reg& address, JIT::Reg& value);
    bool str_l(JIT::Reg& address, JIT::Reg& value);
    bool stxi_w(int& offset, JIT::Reg& address, JIT::Reg& value);
    bool stxi_i(int& offset, JIT::Reg& address, JIT::Reg& value);
    bool stxi_l(int& offset, JIT::Reg& address, JIT::Reg& value);
    bool ret();
    bool retr(JIT::Reg& r);
    bool reti(int& value);

    const uint8_t* getCurrent() const { return m_currentBytecode; }
    bool hasOverflow() const { return m_currentBytecode > m_endOfBytecode; }
    // This can return values larger than |size| in the event of an overflow.
    size_t getSize() const { return m_currentBytecode - m_startOfBytecode; }

private:
    JIT::Reg reg(uint8_t r);
    uint8_t readByte();
    Word readWord();
    UWord readUWord();
    int readInt();

    const uint8_t* m_startOfBytecode = nullptr;
    const uint8_t* m_currentBytecode = nullptr;
    const uint8_t* m_endOfBytecode = nullptr;
};

} // namespace hadron

#endif // SRC_HADRON_OPCODE_ITERATOR_HPP_