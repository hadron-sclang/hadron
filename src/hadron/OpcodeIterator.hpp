#ifndef SRC_HADRON_OPCODE_ITERATOR_HPP_
#define SRC_HADRON_OPCODE_ITERATOR_HPP_

#include "hadron/Arch.hpp"
#include "hadron/JIT.hpp"

#include <cstddef>
#include <cstdint>

namespace hadron {

enum Opcode : int8_t {
    kLoadCArgs2,

    kAddr,
    kAddi,
    kAndi,
    kOri,
    kXorr,
    kMovr,
    kMovi,
    kMovAddr,
    kMoviU,
    kBgei,
    kBeqi,
    kJmp,
    kJmpr,
    kJmpi,
    kLdrL,
    kLdiL,
    kLdxiW,
    kLdxiI,
    kLdxiL,
    kStrI,
    kStrL,
    kStxiW,
    kStxiI,
    kStxiL,
    kRet,

    kInvalid
};

// TODO: why not just have the write iterator implement JIT directly, then cut out VirtualJIT?
class OpcodeWriteIterator {
public:
    OpcodeWriteIterator() = default;
    OpcodeWriteIterator(int8_t* address, size_t size);
    ~OpcodeWriteIterator() = default;

    void setBuffer(int8_t* address, size_t size);
    void reset();

    // All the serialization methods return a boolean if there was capacity to add the element or no.
    bool loadCArgs2(JIT::Reg arg1, JIT::Reg arg2);
    bool addr(JIT::Reg target, JIT::Reg a, JIT::Reg b);
    bool addi(JIT::Reg target, JIT::Reg a, Word b);
    bool andi(JIT::Reg target, JIT::Reg a, UWord b);
    bool ori(JIT::Reg target, JIT::Reg a, UWord b);
    bool xorr(JIT::Reg target, JIT::Reg a, JIT::Reg b);
    bool movr(JIT::Reg target, JIT::Reg value);
    bool movi(JIT::Reg target, Word value);
    bool movi_u(JIT::Reg target, UWord value);
    int8_t* mov_addr(JIT::Reg target);
    // Returns the location of the branch address for later use with patchWord() or nullptr if in overflow.
    int8_t* bgei(JIT::Reg a, Word b);
    int8_t* beqi(JIT::Reg a, Word b);
    int8_t* jmp();
    bool jmpr(JIT::Reg r);
    bool jmpi(UWord location);
    bool ldr_l(JIT::Reg target, JIT::Reg address);
    bool ldi_l(JIT::Reg target, void* address);
    bool ldxi_w(JIT::Reg target, JIT::Reg address, int offset);
    bool ldxi_i(JIT::Reg target, JIT::Reg address, int offset);
    bool ldxi_l(JIT::Reg target, JIT::Reg address, int offset);
    bool str_i(JIT::Reg address, JIT::Reg value);
    bool str_l(JIT::Reg address, JIT::Reg value);
    bool stxi_w(int offset, JIT::Reg address, JIT::Reg value);
    bool stxi_i(int offset, JIT::Reg address, JIT::Reg value);
    bool stxi_l(int offset, JIT::Reg address, JIT::Reg value);
    bool ret();
    // |location| needs to be within the buffer. Overwrites whatever is there with |value|.
    bool patchWord(int8_t* location, Word value);

    int8_t* current() const { return m_currentBytecode; }
    bool hasOverflow() const { return m_currentBytecode > m_endOfBytecode; }
    // This can return values larger than |size| in the event of an overflow.
    size_t getSize() const { return m_currentBytecode - m_startOfBytecode; }

private:
    int8_t reg(JIT::Reg r);
    bool addByte(int8_t byte);
    bool addWord(Word word);
    bool addUWord(UWord word);
    bool addInt(int integer);

    int8_t* m_startOfBytecode = nullptr;
    int8_t* m_currentBytecode = nullptr;
    int8_t* m_endOfBytecode = nullptr;
};

class OpcodeReadIterator {
public:
    OpcodeReadIterator() = default;
    OpcodeReadIterator(const int8_t* buffer, size_t size);
    ~OpcodeReadIterator() = default;

    void setBuffer(const int8_t* address, size_t size);
    void reset();

    // Returns opcode at current() or kInvalid if outside of buffer.
    Opcode peek();
    bool loadCArgs2(JIT::Reg& arg1, JIT::Reg& arg2);
    bool addr(JIT::Reg& target, JIT::Reg& a, JIT::Reg& b);
    bool addi(JIT::Reg& target, JIT::Reg& a, Word& b);
    bool andi(JIT::Reg& target, JIT::Reg& a, UWord& b);
    bool ori(JIT::Reg& target, JIT::Reg& a, UWord& b);
    bool xorr(JIT::Reg& target, JIT::Reg& a, JIT::Reg& b);
    bool movr(JIT::Reg& target, JIT::Reg& value);
    bool movi(JIT::Reg& target, Word& value);
    bool movi_u(JIT::Reg& target, UWord& value);
    bool mov_addr(JIT::Reg& target, const int8_t*& address);
    bool bgei(JIT::Reg& a, Word& b, const int8_t*& address);
    bool beqi(JIT::Reg& a, Word& b, const int8_t*& address);
    bool jmp(const int8_t*& address);
    bool jmpr(JIT::Reg& r);
    bool jmpi(UWord& location);
    bool ldr_l(JIT::Reg& target, JIT::Reg& address);
    bool ldi_l(JIT::Reg& target, void*& address);
    bool ldxi_w(JIT::Reg& target, JIT::Reg& address, int& offset);
    bool ldxi_i(JIT::Reg& target, JIT::Reg& address, int& offset);
    bool ldxi_l(JIT::Reg& target, JIT::Reg& address, int& offset);
    bool str_i(JIT::Reg& address, JIT::Reg& value);
    bool str_l(JIT::Reg& address, JIT::Reg& value);
    bool stxi_w(int& offset, JIT::Reg& address, JIT::Reg& value);
    bool stxi_i(int& offset, JIT::Reg& address, JIT::Reg& value);
    bool stxi_l(int& offset, JIT::Reg& address, JIT::Reg& value);
    bool ret();

    const int8_t* current() const { return m_currentBytecode; }
    void setCurrent(const int8_t* address) { m_currentBytecode = address; }

    bool hasOverflow() const { return m_currentBytecode > m_endOfBytecode; }
    // This can return values larger than |size| in the event of an overflow.
    size_t getSize() const { return m_currentBytecode - m_startOfBytecode; }

private:
    JIT::Reg reg(int8_t r);
    int8_t readByte();
    Word readWord();
    UWord readUWord();
    int readInt();

    const int8_t* m_startOfBytecode = nullptr;
    const int8_t* m_currentBytecode = nullptr;
    const int8_t* m_endOfBytecode = nullptr;
};

} // namespace hadron

#endif // SRC_HADRON_OPCODE_ITERATOR_HPP_