#ifndef SRC_HADRON_CLASS_LIBRARY_HPP_
#define SRC_HADRON_CLASS_LIBRARY_HPP_

#include "hadron/Generator.hpp"
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

namespace hadron {

class Parser;
class SourceFile;
struct ThreadContext;

class ClassLibrary {
public:
    ClassLibrary();
    ~ClassLibrary() = default;

    // Load some minimal information from the classes parsed by schemac during compile time. This allows for fast
    // loading of the interpreter with some information already provided, without parsing the class library. It also
    // allows the interpreter to partially function if the class library compilation is broken.
    void bootstrapLibrary(ThreadContext* context);

    // Scan the input string for class definitions and extensions, performing the first pass of class library
    // compilation. |input| must be valid only for the lifetime of the call. After providing all class definition
    // inputs, call finalizeLibrary() to finish class library compilation.
    bool scanString(ThreadContext* context, std::string_view input, library::Symbol filename);

    template<typename T, typename ...TArgs>
    struct PrimSignature {
    private:
        template<typename W>
        static inline W wrapArg(schema::FramePrivateSchema* framePointer, int32_t& argNumber) {
            auto arg = framePointer->getArg(argNumber);
            ++argNumber;
            return W(arg);
        }
        template<>
        inline int32_t wrapArg<int32_t>(schema::FramePrivateSchema* framePointer, int32_t& argNumber) {
            auto arg = framePointer->getArg(argNumber);
            ++argNumber;
            return arg.getInt32();
        }
    public:
        using functionType = Slot (T::*)(ThreadContext*, TArgs...);

        template<functionType F>
        static constexpr SCMethod makeMethod() {
            return +[](ThreadContext* context, schema::FramePrivateSchema* framePointer, Slot*) -> uint64_t {
                int32_t argNumber = 0;
                auto target = wrapArg<T>(framePointer, argNumber);
                auto value = (target.*F)(context, wrapArg<TArgs>(framePointer, argNumber)...);
                return value.asBits();
            };
        }
    };

    void registerPrimitive(library::Symbol primitiveName, SCMethod method) {
        m_primitives.emplace(std::make_pair(primitiveName, method));
    }

    bool finalizeLibrary(ThreadContext* context);

    library::Class findClassNamed(library::Symbol name) const;
    library::Method functionCompileContext() const { return m_functionCompileContext; }

    library::Array classVariables() const { return m_classVariables; }

    library::Array classArray() const { return m_classArray; }

    static uint64_t dispatch(ThreadContext* context, Hash selectorHash, int32_t numArgs, int32_t numKeyArgs,
                             schema::FramePrivateSchema* callerFrame, Slot* stackPointer);

private:
    // Call to delete any existing class libary compilation structures and start fresh.
    bool resetLibrary(ThreadContext* context);

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

    // A map maintained for quick(er) access to Class objects via Hash. Because the Class objects themselves
    std::unordered_map<library::Symbol, Slot> m_classMap;

    // The official array of Class objects, maintained as part of the root set.
    library::ClassArray m_classArray;

    // All class variables are maintained in a single global array, accessible here.
    library::Array m_classVariables;
    int32_t m_numberOfClassVariables;

    // Outer map is class name to pointer to inner map. Inner map is method name to AST.
    using MethodAST = std::unordered_map<library::Symbol, library::BlockAST>;
    std::unordered_map<library::Symbol, std::unique_ptr<MethodAST>> m_methodASTs;

    using MethodFrame = std::unordered_map<library::Symbol, library::CFGFrame>;
    std::unordered_map<library::Symbol, std::unique_ptr<MethodFrame>> m_methodFrames;

    // Set of class names that are bootstrapped from schema generation, before class library compilation.
    std::unordered_set<library::Symbol> m_bootstrapClasses;

    library::Method m_functionCompileContext;

    std::unordered_map<library::Symbol, SCMethod> m_primitives;
};

} // namespace hadron

#endif // SRC_HADRON_CLASS_LIBRARY_HPP_