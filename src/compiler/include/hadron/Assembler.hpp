#ifndef SRC_COMPILER_INCLUDE_HADRON_ASSEMBLER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_ASSEMBLER_HPP_

#include <memory>
#include <string_view>

namespace hadron {

class ErrorReporter;
class VirtualJIT;

// Mostly used for testing, a bit brittle in terms of human input.
class Assembler {
public:
    Assembler(std::string_view code);
    Assembler(std::string_view code, std::shared_ptr<ErrorReporter> errorReporter);

    ~Assembler() = default;

    bool assemble();

    const VirtualJIT* virtualJIT() const { return m_jit.get(); }

private:
    std::string_view m_code;
    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<VirtualJIT> m_jit;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_ASSEMBLER_HPP_