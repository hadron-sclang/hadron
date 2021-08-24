#ifndef SRC_INCLUDE_HADRON_VIRTUAL_JIT_HPP_
#define SRC_INCLUDE_HADRON_VIRTUAL_JIT_HPP_

#include "hadron/JIT.hpp"

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

    int getRegisterCount() const override;
    int getFloatRegisterCount() const override;

    void addr(Reg target, Reg a, Reg b) override;
    void addi(Reg target, Reg a, int b) override;
    void xorr(Reg target, Reg a, Reg b) override;
    void movr(Reg target, Reg value) override;
    void movi(Reg target, int value) override;
    Label bgei(Reg a, int b) override;
    Label jmp() override;
    void jmpr(Reg r) override;
    void ldxi_w(Reg target, Reg address, int offset) override;
    void ldxi_i(Reg target, Reg address, int offset) override;
    void ldxi_l(Reg target, Reg address, int offset) override;
    void str_i(Reg address, Reg value) override;
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

    enum Opcodes : int32_t {
        kAddr       = 0x0100,
        kAddi       = 0x0200,
        kXorr       = 0x0300,
        kMovr       = 0x0400,
        kMovi       = 0x0500,
        kBgei       = 0x0600,
        kJmp        = 0x0700,
        kJmpR       = 0x0800,
        kLdxiW      = 0x0900,
        kLdxiI      = 0x0a00,
        kLdxiL      = 0x0b00,
        kStrI       = 0x0c00,
        kStxiW      = 0x0d00,
        kStxiI      = 0x0e00,
        kStxiL      = 0x0f00,
        kRet        = 0x1000,
        kRetr       = 0x1100,
        kReti       = 0x1200,
        kEpilog     = 0x1300,
        kLabel      = 0x1400,
        kAddress    = 0x1500,
        kPatchHere  = 0x1600,
        kPatchThere = 0x1700,
    };

    bool toString(std::string& codeString) const;

    using Inst = std::array<int32_t, 4>;
    const std::vector<Inst>& instructions() const { return m_instructions; }

private:
    int m_maxRegisters;
    int m_maxFloatRegisters;
    std::vector<Inst> m_instructions;
    std::vector<size_t> m_labels;  // indices in the m_instructions table.
    int m_addressCount; // keep a count of requests to address() so we can refer to them by index.
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_VIRTUAL_JIT_HPP_