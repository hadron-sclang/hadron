#ifndef SRC_HADRON_CLASS_LIBRARY_HPP_
#define SRC_HADRON_CLASS_LIBRARY_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Slot.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace hadron {

class ErrorReporter;
class Lexer;
class Parser;
class SourceFile;
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

    // Adds a directory to the list of directories to scan for library classes.
    void addClassDirectory(const std::string& path);

    // Scans the provided class directories, builds class inheritance structure. First pass of library compilation.
    bool scanFiles(ThreadContext* context);

private:
    Slot buildClass(ThreadContext* context, Slot filenameSymbol, const hadron::parse::ClassNode* classNode,
            const Lexer* lexer);

    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unordered_map<Hash, library::Class*> m_classTable;

    // We keep the noramlized paths in a set to prevent duplicate additions of the same path.
    std::unordered_set<std::string> m_libraryPaths;
    struct ClassFile {
        std::unique_ptr<SourceFile> sourceFile;
        std::unique_ptr<Lexer> lexer;
        std::unique_ptr<Parser> parser;
    };
    std::unordered_map<std::string, ClassFile> m_classFiles;
};

} // namespace hadron

#endif // SRC_HADRON_CLASS_LIBRARY_HPP_