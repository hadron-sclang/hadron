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
    // Default constructor allows for approximately infinite registers.
    VirtualJIT(std::shared_ptr<ErrorReporter> errorReporter);
    // Constructor for testing allows control over register counts to test register allocation.
    VirtualJIT(std::shared_ptr<ErrorReporter> errorReporter, int maxRegisters, int maxFloatRegisters);
    VirtualJIT() = delete;
    virtual ~VirtualJIT() = default;

    int getRegisterCount() const override;
    int getFloatRegisterCount() const override;

    void addr(Reg target, Reg a, Reg b) override;
    void addi(Reg target, Reg a, int b) override;
    void movr(Reg target, Reg value) override;
    void movi(Reg target, int value) override;
    Label bgei(Reg a, int b) override;
    Label jmpi() override;
    void ldxi_w(Reg target, Reg address, int offset) override;
    void ldxi_i(Reg target, Reg address, int offset) override;
    void ldxi_l(Reg target, Reg address, int offset) override;
    void str_i(Reg address, Reg value) override;
    void sti_i(Address address, Reg value) override;
    void stxi_w(int offset, Reg address, Reg value) override;
    void stxi_i(int offset, Reg address, Reg value) override;
    void stxi_l(int offset, Reg address, Reg value) override;
    void prolog() override;
    Label arg() override;
    void getarg_w(Reg target, Label arg) override;
    void getarg_i(Reg target, Label arg) override;
    void getarg_l(Reg target, Label arg) override;
    void allocai(int stackSizeBytes) override;
    void frame(int stackSizeBytes) override;
    void ret() override;
    void retr(Reg r) override;
    void reti(int value) override;
    void epilog() override;
    Label label() override;
    void patchAt(Label target, Label location) override;
    void patch(Label label) override;

    enum Opcodes : int32_t {
        kAddr    = 0x0100,
        kAddi    = 0x0200,
        kMovr    = 0x0300,
        kMovi    = 0x0400,
        kBgei    = 0x0500,
        kJmpi    = 0x0600,
        kLdxiW   = 0x0700,
        kLdxiI   = 0x0800,
        kLdxiL   = 0x0900,
        kStrI    = 0x0a00,
        kStiI    = 0x0b00,
        kStxiW   = 0x0c00,
        kStxiI   = 0x0d00,
        kStxiL   = 0x0e00,
        kProlog  = 0x0f00,
        kArg     = 0x1000,
        kGetargW = 0x1100,
        kGetargI = 0x1200,
        kGetargL = 0x1300,
        kAllocai = 0x1400,
        kFrame   = 0x1500,
        kRet     = 0x1600,
        kRetr    = 0x1700,
        kReti    = 0x1800,
        kEpilog  = 0x1900,
        kLabel   = 0x1a00,
        kPatchAt = 0x1b00,
        kPatch   = 0x1c00,

        // not JIT opcodes but rather markers to aid rendering.
        kAlias   = 0x1d00,
        kUnalias = 0x1e00
    };

    // mark |r| as active and associated with a given value |name|
    void alias(Reg r);
    void unalias(Reg r);

    bool toString(std::string& codeString) const;

    using Inst = std::array<int32_t, 4>;
    const std::vector<Inst>& instructions() const { return m_instructions; }
    const std::vector<Address>& addresses() const { return m_addresses; }
    // Returns a vector per-register of the indices in instructions() when each register is used.
    const std::vector<std::vector<Label>>& registerUses() const { return m_registerUses; }

private:
    // Add to the register use list for the given virtual register. Returns the same register, for convenience.
    JIT::Reg use(JIT::Reg reg);

    int m_maxRegisters;
    int m_maxFloatRegisters;
    std::vector<Inst> m_instructions;
    std::vector<size_t> m_labels;  // indices in the m_instructions table.
    std::vector<Address> m_addresses;
    std::vector<std::vector<Label>> m_registerUses;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_VIRTUAL_JIT_HPP_