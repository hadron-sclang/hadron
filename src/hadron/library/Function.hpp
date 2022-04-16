#ifndef SRC_HADRON_LIBRARY_FUNCTION_HPP_
#define SRC_HADRON_LIBRARY_FUNCTION_HPP_

#include "hadron/library/Kernel.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/schema/Common/Core/FunctionSchema.hpp"

namespace hadron {
namespace library {

class Function : public Object<Function, schema::FunctionSchema> {
    Function(): Object<Function, schema::FunctionSchema>() {}
    explicit Function(schema::FunctionSchema* instance): Object<Function, schema::FunctionSchema>(instance) {}
    explicit Function(Slot instance): Object<Function, schema::FunctionSchema>(instance) {}
    ~Function() {}

    FunctionDef def() const { return FunctionDef(m_instance->def); }
    void setDef(FunctionDef functionDef) { m_instance->def = functionDef.slot(); }

    Frame context() const { return Frame(m_instance->context); }
    void setContext(Frame frame) { m_instance->context = frame.slot(); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_FUNCTION_HPP_