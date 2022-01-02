#ifndef SRC_COMPILER_INCLUDE_HADRON_VIRTUAL_JIT_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_VIRTUAL_JIT_HPP_

#include "hadron/JIT.hpp"
#include "hadron/OpcodeIterator.hpp"

#include <array>
#include <string>
#include <vector>

namespace hadron {

// Serializes bytecode to a machine-independent three address format that uses virtual registers
class VirtualJIT : public JIT {
public:
    // For unit testing, the empty constructor sets reasonable limits on virtual registers.
    VirtualJIT();
    // Default constructor allows for approximately infinite registers.
    VirtualJIT(std::shared_ptr<ErrorReporter> errorReporter);
    // Constructor for testing allows control over register counts to test register allocation.
    VirtualJIT(std::shared_ptr<ErrorReporter> errorReporter, int maxRegisters, int maxFloatRegisters);
    virtual ~VirtualJIT() = default;

    void begin(uint8_t* buffer, size_t size) override;
    bool hasJITBufferOverflow() override;
    void reset() override;
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
    Label label() override;
    Address address() override;
    void patchHere(Label label) override;
    void patchThere(Label target, Address location) override;

private:
    uint8_t reg(Reg r);

    int m_maxRegisters;
    int m_maxFloatRegisters;
    OpcodeIterator m_iterator;
    std::vector<uint8_t*> m_labels;  // indices in the m_instructions table.
    int m_addressCount; // keep a count of requests to address() so we can refer to them by index.
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_VIRTUAL_JIT_HPP_