#ifndef SRC_HADRON_CLASS_LIBRARY_HPP_
#define SRC_HADRON_CLASS_LIBRARY_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Slot.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace hadron {

class ErrorReporter;
class Lexer;
struct ThreadContext;

namespace library {
struct Class;
}

namespace parse {
struct ClassNode;
struct MethodNode;
} // namespace parse

class ClassLibrary {
public:
    ClassLibrary(std::shared_ptr<ErrorReporter> errorReporter);
    ClassLibrary() = delete;
    ~ClassLibrary() = default;

    // Parse and compile a class file, then add it to the Heap.
    bool addClassFile(ThreadContext* context, const std::string& classFile);

private:
    Slot buildClass(ThreadContext* context, Slot filenameSymbol, const hadron::parse::ClassNode* classNode,
            const Lexer* lexer);
    Slot buildMethod(ThreadContext* context, library::Class* classDef, const hadron::parse::MethodNode* methodNode,
            const Lexer* lexer);

    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unordered_map<Hash, Slot> m_classTable;
};

} // namespace hadron

#endif // SRC_HADRON_CLASS_LIBRARY_HPP_