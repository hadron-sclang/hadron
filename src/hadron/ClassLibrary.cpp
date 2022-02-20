#include "hadron/ClassLibrary.hpp"

#include "hadron/Arch.hpp"
#include "hadron/Block.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
#include "hadron/Emitter.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Hash.hpp"
#include "hadron/Heap.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/LinearFrame.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Pipeline.hpp"
#include "hadron/RegisterAllocator.hpp"
#include "hadron/Resolver.hpp"
#include "hadron/SourceFile.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"
#include "hadron/internal/FileSystem.hpp"

#include "spdlog/spdlog.h"

// Class Library Compilation
// =========================
//
// Because of the ability to compose objects from other objects via inheritance, we have to compile the class library
// in multiple passes:
//
// First Pass: Load input source, lex, and parse. Will need to retain all through next phase of compilation.
//   Can populate the Class tree and add instVarNames, classVarNames, iprototype, cprototype, constNames, constValues,
//   name, nextclass, superclass, and subclasses (by building stubs if the superclass hasn't been prepared yet).
//
// Second Pass: Starting from Object, do pre-order traversal through Object heirarchy tree. Concatenate
//   existing intVarNames, iprototype elements into array containing all superclass data.
//
// Third Pass: Now that the full pedigree and member variables names of each object is known, we compile the individual
//   methods.

namespace hadron {

ClassLibrary::ClassLibrary(std::shared_ptr<ErrorReporter> errorReporter):
    m_errorReporter(errorReporter), m_classArray(nullptr) {}

void ClassLibrary::addClassDirectory(const std::string& path) {
    m_libraryPaths.emplace(fs::absolute(path));
}

bool ClassLibrary::compileLibrary(ThreadContext* context) {
    if (!resetLibrary(context)) { return false; }
    if (!scanFiles(context)) { return false; }
    if (!finalizeHeirarchy(context)) { return false; }
    if (!compileMethods(context)) { return false; }
    return cleanUp();
}

bool ClassLibrary::resetLibrary(ThreadContext* context) {
    m_classMap.clear();
    if (!m_classArray.isNil()) {
        context->heap->removeFromRootSet(m_classArray.slot());
    }
    m_classFiles.clear();
    m_cachedSubclassArrays.clear();

    m_classArray = library::ClassArray::typedArrayAlloc(context, 1);
    context->heap->addToRootSet(m_classArray.slot());
    return true;
}

bool ClassLibrary::scanFiles(ThreadContext* context) {
    for (const auto& classLibPath : m_libraryPaths) {
        for (auto& entry : fs::recursive_directory_iterator(classLibPath)) {
            const auto& path = fs::absolute(entry.path());
            if (!fs::is_regular_file(path) || path.extension() != ".sc")
                continue;
            SPDLOG_INFO("Class Library scanning '{}'", path.c_str());

            auto sourceFile = std::make_unique<SourceFile>(path.string());
            if (!sourceFile->read(m_errorReporter)) { return false; }

            auto lexer = std::make_unique<Lexer>(sourceFile->codeView(), m_errorReporter);
            if (!lexer->lex()) { return false; }

            auto parser = std::make_unique<Parser>(lexer.get(), m_errorReporter);
            if (!parser->parseClass()) { return false; }

            auto filename = library::Symbol::fromView(context, path.string());
            const parse::Node* node = parser->root();
            while (node) {
                // Class extensions can be skipped in first pass, as they don't change the inheritance tree or add
                // any additional member variables.
                if (node->nodeType == parse::NodeType::kClassExt) { continue; }
                // The only other root notes in class files should be ClassNodes.
                if (node->nodeType != parse::NodeType::kClass) {
                    SPDLOG_ERROR("Class file didn't contain Class or Class Extension: {}\n", path.string());
                    return false;
                }

                auto classNode = reinterpret_cast<const parse::ClassNode*>(node);
                if (!scanClass(context, filename, lexer->tokens()[classNode->tokenIndex].range.data() -
                        sourceFile->code(), classNode, lexer.get())) {
                    return false;
                }

                node = classNode->next.get();
            }

            m_classFiles.emplace(std::make_pair(filename,
                    ClassFile{std::move(sourceFile), std::move(lexer), std::move(parser)}));
        }
    }

    return true;
}

bool ClassLibrary::scanClass(ThreadContext* context, library::Symbol filename, int32_t charPos,
        const parse::ClassNode* classNode, const Lexer* lexer) {
    // Compiling a class actually involves generating an instance of the Class object, and then generating an
    // instance of a Meta_ClassName object derived from the Class object, which is where class methods go.
    library::Class classDef = library::Class::alloc(context);
    classDef.initToNil();
    library::Class metaClassDef = library::Class::alloc(context);
    metaClassDef.initToNil();

    SPDLOG_INFO("Class Library compiling class {}", lexer->tokens()[classNode->tokenIndex].range);
    classDef.setName(library::Symbol::fromView(context, lexer->tokens()[classNode->tokenIndex].range));
    metaClassDef.setName(library::Symbol::fromView(context,
            fmt::format("Meta_{}", lexer->tokens()[classNode->tokenIndex].range)));

    if (classNode->superClassNameIndex) {
        classDef.setSuperclass(library::Symbol::fromView(context,
                lexer->tokens()[classNode->superClassNameIndex.value()].range));
        metaClassDef.setSuperclass(library::Symbol::fromView(context, fmt::format("Meta_{}",
                lexer->tokens()[classNode->superClassNameIndex.value()].range)));
    } else {
        if (classDef.name(context).hash() == kObjectHash) {
            // The superclass of 'Meta_Object' is 'Class'.
            metaClassDef.setSuperclass(library::Symbol::fromView(context, "Class"));
        } else {
            classDef.setSuperclass(library::Symbol::fromView(context, "Object"));
            metaClassDef.setSuperclass(library::Symbol::fromView(context, "Meta_Object"));
        }
    }

    // Find and add the class and metaClass to the appropriate superclass object subclasses list.
    if (classDef.name(context).hash() != kObjectHash) {
        // There's no superclass to add to for Object as it has no superclass.
        addToSubclassArray(context, classDef);
    }
    addToSubclassArray(context, metaClassDef);

    classDef.setSubclasses(getSubclassArray(context, classDef));
    metaClassDef.setSubclasses(getSubclassArray(context, metaClassDef));

    classDef.setFilenameSymbol(filename);
    classDef.setCharPos(charPos);
    metaClassDef.setFilenameSymbol(filename);
    metaClassDef.setCharPos(charPos);

    const parse::VarListNode* varList = classNode->variables.get();
    while (varList) {
        auto varHash = lexer->tokens()[varList->tokenIndex].hash;
        library::SymbolArray nameArray;
        library::Array valueArray;

        const parse::VarDefNode* varDef = varList->definitions.get();
        while (varDef) {
            nameArray.add(context, library::Symbol::fromView(context, lexer->tokens()[varDef->tokenIndex].range));
            if (varDef->initialValue) {
                if (varDef->initialValue->nodeType != parse::NodeType::kLiteral) {
                    SPDLOG_ERROR("non-literal initial value in class.");
                    assert(false);
                }
                auto literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
                valueArray.add(context, literal->value);
            } else {
                valueArray.add(context, Slot::makeNil());
            }
            varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        }

        if (varHash == kVarHash) {
            classDef.setInstVarNames(nameArray);
            classDef.setIprototype(valueArray);
        } else if (varHash == kClassVarHash) {
            classDef.setClassVarNames(nameArray);
            classDef.setCprototype(valueArray);
        } else if (varHash == kConstHash) {
            classDef.setConstNames(nameArray);
            classDef.setConstValues(valueArray);
        } else {
            // Internal error with VarListNode pointing at a token that isn't 'var', 'classvar', or 'const'.
            assert(false);
        }

        varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
    }

    m_classMap.emplace(std::make_pair(classDef.name(context), classDef));
    m_classMap.emplace(std::make_pair(metaClassDef.name(context), metaClassDef));
    m_classArray.typedAdd(context, classDef);
    m_classArray.typedAdd(context, metaClassDef);

    return true;
}

// As we encounter the classes in undefined order during the initial scan, we build the subclassses array in each class
// object by adding to it if the class already exists, or by caching an array in m_cachedSubclassArrays if the class
// doesn't exist yet.
void ClassLibrary::addToSubclassArray(ThreadContext* context, const library::Class subclass) {
    auto superclass = subclass.superclass(context);
    auto superclassIter = m_classMap.find(superclass);
    if (superclassIter != m_classMap.end()) {
        auto subclasses = superclassIter->second.subclasses();
        subclasses = subclasses.typedAdd(context, subclass);
        superclassIter->second.setSubclasses(subclasses);
        return;
    }
    auto arrayIter = m_cachedSubclassArrays.find(superclass);
    if (arrayIter != m_cachedSubclassArrays.end()) {
        arrayIter->second.typedAdd(context, subclass);
        return;
    }
    auto subclassArray = library::ClassArray::typedArrayAlloc(context, 1);
    subclassArray.typedAdd(context, subclass);
    m_cachedSubclassArrays.emplace(std::make_pair(superclass, subclassArray));
}

library::ClassArray ClassLibrary::getSubclassArray(ThreadContext* context, const library::Class superclass) {
    // First look in the cached arrays, erase and return if found.
    auto arrayIter = m_cachedSubclassArrays.find(superclass.name(context));
    if (arrayIter != m_cachedSubclassArrays.end()) {
        auto cachedArray = arrayIter->second;
        m_cachedSubclassArrays.erase(arrayIter);
        return cachedArray;
    }

    return library::ClassArray();
}

bool ClassLibrary::finalizeHeirarchy(ThreadContext* context) {
    auto objectIter = m_classMap.find(library::Symbol::fromHash(context, kObjectHash));
    if (objectIter == m_classMap.end()) { assert(false); return false; }

    // We start at the root of the class heirarchy with Object.
    auto objectClassDef = objectIter->second;
    composeSubclassesFrom(context, objectClassDef);

    return true;
}

void ClassLibrary::composeSubclassesFrom(ThreadContext* context, library::Class classDef) {
    for (int32_t i = 0; i < classDef.subclasses().size(); ++i) {
        auto subclass = classDef.subclasses().typedAt(i);
        subclass.setInstVarNames(classDef.instVarNames().copy(
                context, classDef.instVarNames().size() + subclass.instVarNames().size()).addAll(
                context, subclass.instVarNames()));
        subclass.setClassVarNames(classDef.classVarNames().copy(
                context, classDef.classVarNames().size() + subclass.classVarNames().size()).addAll(
                context, subclass.classVarNames()));
        subclass.setIprototype(classDef.iprototype().copy(
                context, classDef.iprototype().size() + subclass.iprototype().size()).addAll(
                context, subclass.iprototype()));
        subclass.setCprototype(classDef.cprototype().copy(
                context, classDef.cprototype().size() + subclass.cprototype().size()).addAll(
                context, subclass.cprototype()));
        subclass.setConstNames(classDef.constNames().copy(
                context, classDef.constNames().size() + subclass.constNames().size()).addAll(
                context, subclass.constNames()));
        subclass.setConstValues(classDef.constValues().copy(
                context, classDef.constValues().size() + subclass.constValues().size()).addAll(
                context, subclass.constValues()));
    }
}

bool ClassLibrary::compileMethods(ThreadContext* /* context */) {
    return true;
}

bool ClassLibrary::cleanUp() {
    m_classFiles.clear();
    m_cachedSubclassArrays.clear();
    return true;
}

} // namespace hadron