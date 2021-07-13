#ifndef SRC_INCLUDE_HADRON_MACHINE_CODE_RENDERER_HPP_
#define SRC_INCLUDE_HADRON_MACHINE_CODE_RENDERER_HPP_

#include <memory>

namespace hadron {

class ErrorReporter;
class JIT;
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
    const VirtualJIT* m_virtualJIT;
    std::unique_ptr<JIT> m_machineJIT;
    std::shared_ptr<ErrorReporter> m_errorReporter;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_MACHINE_CODE_RENDERER_HPP_