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
    Slot value() override;

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
    void ret() override;
    void retr(Reg r) override;
    void epilog() override;
    Label label() override;
    void patchAt(Label target, Label location) override;
    void patch(Label label) override;

    enum Opcodes : int32_t {
        kAddr,
        kAddi,
        kMovr,
        kMovi,
        kBgei,
        kJmpi,
        kLdxi,
        kStr,
        kSti,
        kStxi,
        kProlog,
        kArg,
        kGetarg,
        kAllocai,
        kRet,
        kRetr,
        kEpilog,
        kLabel,
        kPatchAt,
        kPatch,

        // not JIT opcodes but rather markers to aid rendering.
        kAlias,
        kUnalias
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