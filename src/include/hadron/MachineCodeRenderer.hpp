#ifndef SRC_INCLUDE_HADRON_MACHINE_CODE_RENDERER_HPP_
#define SRC_INCLUDE_HADRON_MACHINE_CODE_RENDERER_HPP_

#include "hadron/JIT.hpp"

#include <unordered_map>
#include <memory>
#include <set>
#include <vector>

namespace hadron {

class ErrorReporter;
class VirtualJIT;

// A machine code renderer will take code from a VirtualJIT object, assign registers, and JIT the output code.
class MachineCodeRenderer {
public:
    MachineCodeRenderer(const VirtualJIT* virtualJIT, std::shared_ptr<ErrorReporter> errorReporter);
    MachineCodeRenderer() = delete;
    ~MachineCodeRenderer() = default;

    bool render();

    const JIT* machineJIT() const { return m_machineJIT.get(); }

private:
    // We create distinct types between virtual and machine registers, to help with clarity in the code. But they are
    // all the same underlying JIT::Reg type.
    using MReg = JIT::Reg;
    using VReg = JIT::Reg;

    const VirtualJIT* m_virtualJIT;
    std::unique_ptr<JIT> m_machineJIT;
    std::shared_ptr<ErrorReporter> m_errorReporter;

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
     * are tracked as keys in the m_spilledRegisters map, with the index into the stack area as value.
     *
     * Machine registers move through a simpler state diagram:
     *
     *      free <===> allocated
     *
     * Spilling and unspilling virtual registers doesn't change the 'allocated' state of the target machine register,
     * only the association between virtual registers and target machine register. Free machine registers are tracked
     * in the m_freeRegisters set. This is an ordered set to allow for stability in register assignment but also
     * because on LightningJIT at least the lower-numbered registers are caller-save and so may be cheaper to allocate
     * across function calls.
     *
     * Optimal spilling strategy is an NP-Hard computational problem. This heuristic is a naive "bottom-up" spilling
     * algorithm that chooses the register to spill based on furthest use from the current location. This strategy
     * gives reasonable overall performance but over time may require further refinement.
     *
     */

    // Immediately allocate a machine register for the supplied virtual register, issuing spill code if needed.
    void allocateRegister(VReg vReg);
    // Returns the allocated machine register for the supplied virtual register. May result in spill and/or unspill
    // code to make room.
    MReg mReg(VReg vReg);
    // Free the underlying machine register associated with this virtual register.
    void freeRegister(VReg vReg);
    // Select the most appropriate allocated virutal register and spill it, returning the freed machine register.
    MReg spill();

    std::vector<JIT::Label> m_labels;
    // How many register-size spaces to reserve on the stack for register spilling.
    int m_spillStackSize;
    // We start the spill stack after whatever the virtual JIT stack has reserved, so we store the size of the
    // virtual JIT stack here to start the spill stack from.
    int m_spillStackOffsetBytes;
    // Map of virtual registers to machine registers.
    std::unordered_map<VReg, MReg> m_allocatedRegisters;
    // The set of free machine registers.  // TODO: these sets could very easily be min heaps.
    std::set<MReg> m_freeRegisters;
    // Map of spilled virtual registers to spill stack space index.
    std::unordered_map<VReg, int> m_spilledRegisters;
    // The set of free spill indices.
    std::set<int> m_freeSpillIndices;
    // For each virtual register n this vector contains the index in the nth virtualJIT uses() register for the *next*
    // use of the register in the machine code, or size_t if the virtual register is no longer used.
    std::vector<size_t> m_useCursors;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_MACHINE_CODE_RENDERER_HPP_