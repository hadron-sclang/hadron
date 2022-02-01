#ifndef SRC_HADRON_CLASS_LIBRARY_HPP_
#define SRC_HADRON_CLASS_LIBRARY_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Slot.hpp"
#include "hadron/library/Array.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/library/Symbol.hpp"

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

    // Compile the class library by scanning all directories added with addClassDirectory() and compile all class files
    // found within.
    bool compileLibrary(ThreadContext* context);

private:
    // Call to delete any existing class libary compilation structures and start fresh.
    bool resetLibrary(ThreadContext* context);

    // Scans the provided class directories, builds class inheritance structure. First pass of library compilation.
    bool scanFiles(ThreadContext* context);
    bool scanClass(ThreadContext* context, library::Symbol filename, int32_t charPos,
            const hadron::parse::ClassNode* classNode, const Lexer* lexer);
    // Adds subClass to the existing superclass object if it exists, or caches a new list of subclasses if it does not.
    void addToSubclassArray(ThreadContext* context, const library::Class subclass);
    // Returns existing array if cached, or nil if not.
    library::ClassArray getSubclassArray(ThreadContext* context, const library::Class superclass);

    // Traverse the class tree in superclass to subclass order, starting with Object, and finalize all inherited
    // properties.
    bool finalizeHeirarchy(ThreadContext* context);
    void composeSubclassesFrom(ThreadContext* context, library::Class classDef);

    // Compile all of the provided methods for all classes.
    bool compileMethods(ThreadContext* context);

    // Clean up any temporary data structures
    bool cleanUp();

    std::shared_ptr<ErrorReporter> m_errorReporter;

    // A map maintained for quick(er) access to Class objects via Hash.
    std::unordered_map<library::Symbol, library::Class> m_classMap;

    // The official array of Class objects, maintained as part of the root set.
    library::ClassArray m_classArray;

    // We keep the noramlized paths in a set to prevent duplicate additions of the same path.
    std::unordered_set<std::string> m_libraryPaths;
    struct ClassFile {
        std::unique_ptr<SourceFile> sourceFile;
        std::unique_ptr<Lexer> lexer;
        std::unique_ptr<Parser> parser;
    };
    std::unordered_map<library::Symbol, ClassFile> m_classFiles;
    std::unordered_map<library::Symbol, library::ClassArray> m_cachedSubclassArrays;
};

} // namespace hadron

#endif // SRC_HADRON_CLASS_LIBRARY_HPP_