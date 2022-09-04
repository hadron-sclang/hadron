#ifndef SRC_HADRON_VIRTUAL_MACHINE_HPP_
#define SRC_HADRON_VIRTUAL_MACHINE_HPP_

#include "hadron/Arch.hpp"
#include "hadron/JIT.hpp"
#include "hadron/library/ArrayedCollection.hpp"

#include <array>

namespace hadron {

struct ThreadContext;

// Represents a simulated simple computer that can execute machine instructions issued from the VirtualJIT. Useful
// for step-by-step debugging and validation of the emitted bytecode.
class VirtualMachine {
public:
    // Makes a VM with the same number of registers as the host computer.
    VirtualMachine() = default;
    ~VirtualMachine() = default;

    void executeMachineCode(ThreadContext* context, const int8_t* entryCode, const int8_t* targetCode);

private:
    bool readGPR(JIT::Reg reg, UWord& value);
    bool writeGPR(JIT::Reg reg, UWord value);
    bool checkAddress(ThreadContext* context, UWord addr);
    library::Int8Array resolveCodePointer(ThreadContext* context, const int8_t* addr);

    std::array<UWord, kNumberOfPhysicalRegisters> m_gprs;
    std::array<int8_t, kNumberOfPhysicalRegisters> m_setGPRs;
};

} // namespace hadron

#endif