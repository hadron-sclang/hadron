#ifndef SRC_COMPILER_INCLUDE_HADRON_VIRTUAL_JIT_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_VIRTUAL_JIT_HPP_

#include "hadron/JIT.hpp"
#include "hadron/OpcodeIterator.hpp"

#include <vector>

namespace hadron {

// Serializes bytecode to a machine-independent three address format that uses virtual registers
class VirtualJIT : public JIT {
public:
    // For unit testing, the empty constructor sets reasonable limits on virtual registers.
    VirtualJIT();
    // Constructor for testing allows control over register counts to test register allocation.
    VirtualJIT(int maxRegisters, int maxFloatRegisters);
    virtual ~VirtualJIT() = default;

    void begin(int8_t* buffer, size_t size) override;
    bool hasJITBufferOverflow() override;
    void reset() override;
    // Because the iterator can continue to record sizes even after overflow, if the buffer has overflowed this will
    // return a |sizeOut| greater than the supplied buffer size.
    Address end(size_t* sizeOut) override;
    int getRegisterCount() const override;
    int getFloatRegisterCount() const override;

    void addr(Reg target, Reg a, Reg b) override;
    void addi(Reg target, Reg a, Word b) override;
    void andi(Reg target, Reg a, UWord b) override;
    void ori(Reg target, Reg a, UWord b) override;
    void xorr(Reg target, Reg a, Reg b) override;
    void movr(Reg target, Reg value) override;
    void movi(Reg target, Word value) override;
    Label bgei(Reg a, Word b) override;
    Label beqi(Reg a, Word b) override;
    Label jmp() override;
    void jmpr(Reg r) override;
    void jmpi(Address location) override;
    void ldr_l(Reg target, Reg address) override;
    void ldxi_w(Reg target, Reg address, int offset) override;
    void ldxi_i(Reg target, Reg address, int offset) override;
    void ldxi_l(Reg target, Reg address, int offset) override;
    void str_i(Reg address, Reg value) override;
    void str_l(Reg address, Reg value) override;
    void stxi_w(int offset, Reg address, Reg value) override;
    void stxi_i(int offset, Reg address, Reg value) override;
    void stxi_l(int offset, Reg address, Reg value) override;
    void ret() override;
    void retr(Reg r) override;
    void reti(int value) override;
    Address address() override;
    void patchHere(Label label) override;
    void patchThere(Label target, Address location) override;

private:
    int m_maxRegisters;
    int m_maxFloatRegisters;
    OpcodeWriteIterator m_iterator;
    // Labels are pointers into the bytecode with room reserved for patching the bytecode with address values.
    std::vector<int8_t*> m_labels;
    // Addresses are pointers into the bytecode with no room reserved for patching the bytecode.
    std::vector<int8_t*> m_addresses;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_VIRTUAL_JIT_HPP_