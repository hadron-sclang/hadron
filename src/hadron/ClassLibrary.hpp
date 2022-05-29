#ifndef SRC_HADRON_CLASS_LIBRARY_HPP_
#define SRC_HADRON_CLASS_LIBRARY_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Slot.hpp"
#include "hadron/library/Array.hpp"
#include "hadron/library/HadronAST.hpp"
#include "hadron/library/HadronCFG.hpp"
#include "hadron/library/HadronHIR.hpp"
#include "hadron/library/HadronParseNode.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/library/Symbol.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace hadron {

class ErrorReporter;
class Parser;
class SourceFile;
struct ThreadContext;

class ClassLibrary {
public:
    ClassLibrary() = delete;
    explicit ClassLibrary(std::shared_ptr<ErrorReporter> errorReporter);
    ~ClassLibrary() = default;

    // Adds a directory to the list of directories to scan for library classes.
    void addClassDirectory(const std::string& path);

    // Compile the class library by scanning all directories added with addClassDirectory() and compile all class files
    // found within.
    bool compileLibrary(ThreadContext* context);

    library::Class findClassNamed(library::Symbol name) const;

    library::Method interpreterContext() const { return m_interpreterContext; }

    library::Array classVariables() const { return m_classVariables; }

    library::Array classArray() const { return m_classArray; }

private:
    // Call to delete any existing class libary compilation structures and start fresh.
    bool resetLibrary(ThreadContext* context);

    // Scans the provided class directories, builds class inheritance structure. First pass of library compilation.
    bool scanFiles(ThreadContext* context);
    bool scanClass(ThreadContext* context, library::Class classDef, library::Class metaClassDef,
            const library::ClassNode classNode);
    // Either create a new Class object with the provided name, or return the existing one.
    library::Class findOrInitClass(ThreadContext* context, library::Symbol className);

    // Traverse the class tree in superclass to subclass order, starting with Object, and finalize all inherited
    // properties, plus lower from AST to Frame representation.
    bool finalizeHeirarchy(ThreadContext* context);
    bool composeSubclassesFrom(ThreadContext* context, library::Class classDef);

    // Finish compilation from Frame down to executable bytecode.
    bool materializeFrames(ThreadContext* context);

    // Clean up any temporary data structures
    bool cleanUp();

    std::shared_ptr<ErrorReporter> m_errorReporter;
    // We keep the normalized paths in a set to prevent duplicate additions of the same path.
    std::unordered_set<std::string> m_libraryPaths;

    // A map maintained for quick(er) access to Class objects via Hash.
    std::unordered_map<library::Symbol, library::Class> m_classMap;

    // The official array of Class objects, maintained as part of the root set.
    library::ClassArray m_classArray;

    // We keep a reference to the Interpreter:functionCompileContext method, as that is the "fake method" that
    // all Interpreter code compiles as.
    library::Method m_interpreterContext;

    // All class variables are maintained in a single global array, accessible here.
    library::Array m_classVariables;
    int32_t m_numberOfClassVariables;

    // Outer map is class name to pointer to inner map. Inner map is method name to AST.
    using MethodAST = std::unordered_map<library::Symbol, library::BlockAST>;
    std::unordered_map<library::Symbol, std::unique_ptr<MethodAST>> m_methodASTs;

    using MethodFrame = std::unordered_map<library::Symbol, library::CFGFrame>;
    std::unordered_map<library::Symbol, std::unique_ptr<MethodFrame>> m_methodFrames;
};

} // namespace hadron

#endif // SRC_HADRON_CLASS_LIBRARY_HPP_