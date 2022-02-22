#include "hadron/ClassLibrary.hpp"

#include "hadron/Arch.hpp"
#include "hadron/AST.hpp"
#include "hadron/ASTBuilder.hpp"
#include "hadron/Block.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
#include "hadron/Emitter.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Hash.hpp"
#include "hadron/Heap.hpp"
#include "hadron/internal/FileSystem.hpp"
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

#include "spdlog/spdlog.h"

#include <cassert>

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
    if (!buildFrames(context)) { return false; }
    return cleanUp();
}

bool ClassLibrary::resetLibrary(ThreadContext* context) {
    m_classMap.clear();
    m_classArray = library::ClassArray::typedArrayAlloc(context, 1);
    m_classMethods.clear();
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
                if (node->nodeType != parse::NodeType::kClass && node->nodeType != parse::NodeType::kClassExt) {
                    SPDLOG_ERROR("Expecting either Class or Class Extensions only at top level in class file {}",
                            path.c_str());
                    return false;
                }

                library::Symbol className = library::Symbol::fromView(context, lexer->tokens()[node->tokenIndex].range);
                library::Class classDef = findOrInitClass(context, className);

                library::Symbol metaClassName = library::Symbol::fromView(context, fmt::format("Meta_{}",
                        lexer->tokens()[node->tokenIndex].range));
                library::Class metaClassDef = findOrInitClass(context, metaClassName);

                const parse::MethodNode* methodNode = nullptr;

                if (node->nodeType == parse::NodeType::kClass) {
                    const auto classNode = reinterpret_cast<const parse::ClassNode*>(node);

                    int32_t charPos = lexer->tokens()[classNode->tokenIndex].range.data() - sourceFile->code();
                    classDef.setFilenameSymbol(filename);
                    classDef.setCharPos(charPos);
                    metaClassDef.setFilenameSymbol(filename);
                    metaClassDef.setCharPos(charPos);

                    if (!scanClass(context, classDef, metaClassDef, classNode, lexer.get())) {
                        return false;
                    }

                    methodNode = classNode->methods.get();
                } else {
                    assert(node->nodeType == parse::NodeType::kClassExt);
                    const auto classExtNode = reinterpret_cast<const parse::ClassExtNode*>(node);
                    methodNode = classExtNode->methods.get();
                }

                while (methodNode) {
                    assert(methodNode->nodeType == parse::NodeType::kMethod);
                    library::Method method = library::Method::alloc(context);

                    library::Class methodClassDef = methodNode->isClassMethod ? metaClassDef : classDef;
                    method.setOwnerClass(methodClassDef);

                    library::MethodArray methodArray = methodClassDef.methods();
                    methodArray = methodArray.typedAdd(context, method);
                    methodClassDef.setMethods(methodArray);

                    library::Symbol methodName = library::Symbol::fromView(context,
                            lexer->tokens()[methodNode->tokenIndex].range);
                    method.setName(methodName);

                    if (methodNode->primitiveIndex) {
                        library::Symbol primitiveName = library::Symbol::fromView(context,
                            lexer->tokens()[*methodNode->primitiveIndex].range);
                        method.setPrimitiveName(primitiveName);
                    } else {
                        SPDLOG_INFO("Building AST for {}:{}", methodClassDef.name(context).view(context),
                                methodName.view(context));
                        // Build the AST from the MethodNode block.
                        ASTBuilder astBuilder(m_errorReporter);
                        auto ast = astBuilder.buildBlock(context, lexer.get(), methodNode->body.get());
                        if (!ast) { return false; }
                        auto methodIter = m_classMethods.find(methodClassDef.name(context));
                        assert(methodIter != m_classMethods.end());
                        methodIter->second->emplace(std::make_pair(methodName, std::move(ast)));
                    }

                    method.setFilenameSymbol(filename);

                    int32_t charPos = lexer->tokens()[methodNode->tokenIndex].range.data() - sourceFile->code();
                    method.setCharPos(charPos);

                    methodNode = reinterpret_cast<const parse::MethodNode*>(methodNode->next.get());
                }

                node = node->next.get();
            }
        }
    }

    return true;
}

bool ClassLibrary::scanClass(ThreadContext* context, library::Class classDef, library::Class metaClassDef,
        const parse::ClassNode* classNode, const Lexer* lexer) {

    library::Symbol superclassName;
    library::Symbol metaSuperclassName;

    if (classNode->superClassNameIndex) {
        superclassName = library::Symbol::fromView(context,
                lexer->tokens()[classNode->superClassNameIndex.value()].range);
        metaSuperclassName = library::Symbol::fromView(context, fmt::format("Meta_{}",
                lexer->tokens()[classNode->superClassNameIndex.value()].range));
    } else {
        if (classDef.name(context).hash() == kObjectHash) {
            // The superclass of 'Meta_Object' is 'Class'.
            metaSuperclassName = library::Symbol::fromView(context, "Class");
        } else {
            superclassName = library::Symbol::fromView(context, "Object");
            metaSuperclassName = library::Symbol::fromView(context, "Meta_Object");
        }
    }

    // Set up parent object and add this class definition to its subclasses array, if this isn't `Object`.
    if (classDef.name(context).hash() != kObjectHash) {
        classDef.setSuperclass(superclassName);
        library::Class superclass = findOrInitClass(context, superclassName);
        library::ClassArray subclasses = superclass.subclasses();
        subclasses = subclasses.typedAdd(context, classDef);
        superclass.setSubclasses(subclasses);
    }

    // Set up the parent object for the Meta class, which always has a parent.
    metaClassDef.setSuperclass(metaSuperclassName);
    library::Class metaSuperclass = findOrInitClass(context, metaSuperclassName);
    library::ClassArray metaSubclasses = metaSuperclass.subclasses();
    metaSubclasses = metaSubclasses.typedAdd(context, metaClassDef);
    metaSuperclass.setSubclasses(metaSubclasses);

    // Extract class and instance variables and constants.
    const parse::VarListNode* varList = classNode->variables.get();
    while (varList) {
        auto varHash = lexer->tokens()[varList->tokenIndex].hash;
        library::SymbolArray nameArray;
        library::Array valueArray;

        const parse::VarDefNode* varDef = varList->definitions.get();
        while (varDef) {
            nameArray = nameArray.add(context, library::Symbol::fromView(context,
                    lexer->tokens()[varDef->tokenIndex].range));
            if (varDef->initialValue) {
                if (varDef->initialValue->nodeType != parse::NodeType::kLiteral) {
                    SPDLOG_ERROR("non-literal initial value in class.");
                    assert(false);
                }
                auto literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
                valueArray = valueArray.add(context, literal->value);
            } else {
                valueArray = valueArray.add(context, Slot::makeNil());
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

    return true;
}

library::Class ClassLibrary::findOrInitClass(ThreadContext* context, library::Symbol className) {
    auto iter = m_classMap.find(className);
    if (iter != m_classMap.end()) {
        return iter->second;
    }

    library::Class classDef = library::Class::alloc(context);
    classDef.initToNil();
    classDef.setName(className);

    m_classMap.emplace(std::make_pair(className, classDef));

    if (m_classArray.size()) {
        classDef.setNextclass(m_classArray.typedAt(m_classArray.size() - 1));
    }
    m_classArray = m_classArray.typedAdd(context, classDef);

    // Add an empty entry to the class methods map, to keep membership in that map in sync with the class map.
    m_classMethods.emplace(std::make_pair(className, std::make_unique<ClassLibrary::MethodAST>()));

    return classDef;
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
        composeSubclassesFrom(context, subclass);
    }
}

bool ClassLibrary::buildFrames(ThreadContext* /* context */) {
    for (const auto& classMethods : m_classMethods) {
        auto classIter = m_classMap.find(classMethods.first);
        assert(classIter != m_classMap.end());
        auto classDef = classIter->second;
    }

    return true;
}

bool ClassLibrary::cleanUp() {
    return true;
}

} // namespace hadron