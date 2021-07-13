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
    VirtualJIT() = default;
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

private:
    std::vector<Inst> m_instructions;
    std::vector<size_t> m_labels;  // indices in the m_instructions table.
    std::vector<Address> m_addresses;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_VIRTUAL_JIT_HPP_