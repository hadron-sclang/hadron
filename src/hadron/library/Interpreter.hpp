#ifndef SRC_HADRON_LIBRARY_INTERPRETER_HPP_
#define SRC_HADRON_LIBRARY_INTERPRETER_HPP_

#include "hadron/library/Function.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/schema/Common/Core/KernelSchema.hpp"

namespace hadron {
namespace library {

class Interpreter : public Object<Interpreter, schema::InterpreterSchema> {
public:
    Interpreter(): Object<Interpreter, schema::InterpreterSchema>() {}
    explicit Interpreter(schema::InterpreterSchema* instance):
        Object<Interpreter, schema::InterpreterSchema>(instance) {}
    explicit Interpreter(Slot instance):
        Object<Interpreter, schema::InterpreterSchema>(instance) {}
    ~Interpreter() {}

    Function compile(ThreadContext* context, String string);
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_INTERPRETER_HPP_