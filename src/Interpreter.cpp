#include "hadron/Interpreter.hpp"

#include "hadron/Compiler.hpp"
#include "hadron/LighteningJIT.hpp"

namespace hadron {

Interpreter::Interpreter(): m_compiler(std::make_unique<Compiler>()) { }

Interpreter::~Interpreter() {
    stop();
}

bool Interpreter::start() {
    if (!m_compiler->start()) {
        return false;
    }

    LighteningJIT::markThreadForJITCompilation();
    LighteningJIT jit;
}

void Interpreter::stop() {

}

std::unique_ptr<Function> Interpreter::compile(std::string_view code) {

}

std::unique_ptr<Function> Interpreter::compileFile() {

}

Slot Interpreter::run(Function* func) {

}


}  // namespace hadron