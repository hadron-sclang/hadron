#ifndef SRC_INCLUDE_HADRON_MACHINE_CODE_RENDERER_HPP_
#define SRC_INCLUDE_HADRON_MACHINE_CODE_RENDERER_HPP_

#include "hadron/JIT.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hadron {

class ErrorReporter;
class LighteningJIT;
class VirtualJIT;

// A machine code renderer will take code from a VirtualJIT object, assign registers, and JIT the output code.
class MachineCodeRenderer {
public:
    MachineCodeRenderer(const VirtualJIT* virtualJIT, std::shared_ptr<ErrorReporter> errorReporter);
    MachineCodeRenderer() = delete;
    ~MachineCodeRenderer() = default;

    // Interate over the instructions in virtualJIT, perform register fitting, and issue the modified instructions
    // into the provided jit object.
    bool render(JIT* jit);

private:
    // We create distinct types between virtual and machine registers, to help with clarity in the code. But they are
    // all the same underlying JIT::Reg type.
    using MReg = JIT::Reg;
    using VReg = JIT::Reg;

    /*
     *  Virtual Registers move through states as follows:
     *
     *      free <===> allocated <===> spilled
     *
     * Meaning that transitions to and from 'free' to 'allocated', and to and from 'allocated' to 'spilled', are the
     * only valid state transitions allowed for virtual registers. To 'allocate' a virtual register means to assign
     * it to an actual machine register. To 'spill' a virtual register is to save the contents of the allocated machine
     * register out to the stack, to make room for another virtual register to be allocated. To 'unspill' a virtual
     * register is to load it back into a (possibly different than where it was when spilled) machine register, changing
     * the state back to allocated.
     *
     * Free virtual registers aren't tracked in the system. Allocated virtual registers are keys in the
     * m_allocatedRegisters map, with the associated machine register as the value. Spilled virtual registers
     * are tracked as keys in the m_spilledRegisters set. Virtual registers are always spilled into and out of their
     * index in order on the stack.
     *
     * Machine registers move through a simpler state diagram:
     *
     *      free <===> allocated
     *
     * Spilling and unspilling virtual registers doesn't change the 'allocated' state of the target machine register,
     * only the association between virtual registers and target machine register. Free machine registers are tracked
     * in the m_freeRegisters min heap. This is ordered to allow for stability in register assignment.
     */

    // Immediately allocate a machine register for the supplied virtual register, issuing spill code if needed.
    void allocateRegister(VReg vReg, JIT* jit);
    // Returns the allocated machine register for the supplied virtual register. May result in spill and/or unspill
    // code to make room.
    MReg mReg(VReg vReg, JIT* jit);
    // Free the underlying machine register associated with this virtual register.
    void freeRegister(VReg vReg);
    // Select the most appropriate allocated virutal register and spill it, returning the freed machine register.
    MReg spill(JIT* jit);

    const VirtualJIT* m_virtualJIT;
    std::shared_ptr<ErrorReporter> m_errorReporter;

    // Size in slots of the register spill area, useful for advancing the stack pointer past this in function calls.
    int m_spillAreaSize;

    std::vector<JIT::Label> m_labels;
    // Map of allocated virtual registers to machine registers.
    std::unordered_map<VReg, MReg> m_allocatedRegisters;
    // A min heap of free register numbers, for stable ordering in allocation.
    std::vector<MReg> m_freeRegisters;
    // Set of spilled registers.
    std::unordered_set<VReg> m_spilledRegisters;
    // For each virtual register n this vector contains the index in the nth virtualJIT uses() register for the *next*
    // use of the register in the machine code, or size of that array if the virtual register is no longer used.
    std::vector<size_t> m_useCursors;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_MACHINE_CODE_RENDERER_HPP_