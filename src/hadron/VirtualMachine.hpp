#ifndef SRC_HADRON_VIRTUAL_MACHINE_HPP_
#define SRC_HADRON_VIRTUAL_MACHINE_HPP_

namespace hadron {

// Represents a simulated simple computer that can execute machine instructions issued from the VirtualJIT. Useful
// for step-by-step debugging and validation of the emitted bytecode.
class VirtualMachine {
public:
    // Makes a VM with the same number of registers as the host computer.
    VirtualMachine();
    ~VirtualMachine();
};

} // namespace hadron

#endif