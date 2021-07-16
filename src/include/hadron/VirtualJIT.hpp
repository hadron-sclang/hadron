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

    bool emit() override;
    bool evaluate(Slot* value) const override;
    void print() const override;

    int getRegisterCount() const override;
    int getFloatRegisterCount() const override;

    void addr(Reg target, Reg a, Reg b) override;
    void addi(Reg target, Reg a, int b) override;
    void movr(Reg target, Reg value) override;
    void movi(Reg target, int value) override;
    Label bgei(Reg a, int b) override;
    Label jmpi() override;
    void ldxi(Reg target, Reg address, int offset) override;
    void str(Reg address, Reg value) override;
    void sti(Address address, Reg value) override;
    void stxi(int offset, Reg address, Reg value) override;
    void prolog() override;
    Label arg() override;
    void getarg(Reg target, Label arg) override;
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
        kLdxi    = 0x0700,
        kStr     = 0x0800,
        kSti     = 0x0900,
        kStxi    = 0x0a00,
        kProlog  = 0x0b00,
        kArg     = 0x0c00,
        kGetarg  = 0x0d00,
        kAllocai = 0x0e00,
        kFrame   = 0x0f00,
        kRet     = 0x1000,
        kRetr    = 0x1100,
        kReti    = 0x1200,
        kEpilog  = 0x1300,
        kLabel   = 0x1400,
        kPatchAt = 0x1500,
        kPatch   = 0x1600,

        // not JIT opcodes but rather markers to aid rendering.
        kAlias   = 0x1700,
        kUnalias = 0x1800
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