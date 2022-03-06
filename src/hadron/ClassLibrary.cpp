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
#include "hadron/RegisterAllocator.hpp"
#include "hadron/Resolver.hpp"
#include "hadron/SourceFile.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"
#include "hadron/Validator.hpp"

#include "spdlog/spdlog.h"

#include <cassert>

namespace hadron {

ClassLibrary::ClassLibrary(std::shared_ptr<ErrorReporter> errorReporter):
        m_errorReporter(errorReporter),
        m_classArray(nullptr) {}

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

library::Class ClassLibrary::findClassNamed(library::Symbol name) const {
    if (name.isNil()) { return library::Class(); }
    auto classIter = m_classMap.find(name);
    if (classIter == m_classMap.end()) { return library::Class(); }
    return classIter->second;
}

bool ClassLibrary::resetLibrary(ThreadContext* context) {
    m_classMap.clear();
    m_classArray = library::ClassArray::typedArrayAlloc(context, 1);
    m_methodASTs.clear();
    m_methodFrames.clear();
    m_interpreterContext = library::Method();
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
                    method.initToNil();

                    library::Class methodClassDef = methodNode->isClassMethod ? metaClassDef : classDef;
                    method.setOwnerClass(methodClassDef);

                    library::MethodArray methodArray = methodClassDef.methods();
                    methodArray = methodArray.typedAdd(context, method);
                    methodClassDef.setMethods(methodArray);

                    library::Symbol methodName = library::Symbol::fromView(context,
                            lexer->tokens()[methodNode->tokenIndex].range);
                    method.setName(methodName);

                    // We keep a reference to the Interpreter compile context, for quick access later.
                    if ((className == library::Symbol::fromView(context, "Interpreter")) &&
                        (methodName == library::Symbol::fromView(context, "functionCompileContext"))) {
                        m_interpreterContext = method;
                    }

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
                        auto methodIter = m_methodASTs.find(methodClassDef.name(context));
                        assert(methodIter != m_methodASTs.end());
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

    SPDLOG_INFO("Scanning Class: {}", classDef.name(context).view(context));

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
        auto nameArray = library::SymbolArray();
        auto valueArray = library::Array();

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

        // Each line gets its own varList parse node, so append to any existing arrays to preserve previous values.
        if (varHash == kVarHash) {
            classDef.setInstVarNames(classDef.instVarNames().addAll(context, nameArray));
            classDef.setIprototype(classDef.iprototype().addAll(context, valueArray));
        } else if (varHash == kClassVarHash) {
            classDef.setClassVarNames(classDef.classVarNames().addAll(context, nameArray));
            classDef.setCprototype(classDef.cprototype().addAll(context, valueArray));
        } else if (varHash == kConstHash) {
            classDef.setConstNames(classDef.constNames().addAll(context, nameArray));
            classDef.setConstValues(classDef.constValues().addAll(context, valueArray));
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

    // Add an empty entry to the class methods maps, to keep membership in that map in sync with the class map.
    m_methodASTs.emplace(std::make_pair(className, std::make_unique<ClassLibrary::MethodAST>()));
    m_methodFrames.emplace(std::make_pair(className, std::make_unique<ClassLibrary::MethodFrame>()));

    return classDef;
}

bool ClassLibrary::finalizeHeirarchy(ThreadContext* context) {
    auto objectIter = m_classMap.find(library::Symbol::fromView(context, "Object"));
    if (objectIter == m_classMap.end()) { assert(false); return false; }

    // We start at the root of the class heirarchy with Object.
    auto objectClassDef = objectIter->second;
    return composeSubclassesFrom(context, objectClassDef);
}

bool ClassLibrary::composeSubclassesFrom(ThreadContext* context, library::Class classDef) {
    SPDLOG_INFO("Composing Class {}", classDef.name(context).view(context));

    auto classASTs = m_methodASTs.find(classDef.name(context));
    assert(classASTs != m_methodASTs.end());

    auto classMethods = m_methodFrames.find(classDef.name(context));
    assert(classMethods != m_methodFrames.end());

    for (int32_t i = 0; i < classDef.methods().size(); ++i) {
        auto method = classDef.methods().typedAt(i);

        // We don't compile methods that include primitives.
        if (!method.primitiveName(context).isNil()) { continue; }

        auto methodName = method.name(context);

        SPDLOG_INFO("Building Frame for {}:{}", classDef.name(context).view(context), methodName.view(context));

        auto astIter = classASTs->second->find(methodName);
        assert(astIter != classASTs->second->end());

        BlockBuilder blockBuilder(m_errorReporter);
        auto frame = blockBuilder.buildMethod(context, method, astIter->second.get());
        if (!frame) { return false; }

        if (context->runInternalDiagnostics) {
            if (!Validator::validateFrame(frame.get())) { return false; }
        }

        // TODO: Here's where we could extract some message signatures and compute dependencies, to decide on final
        // ordering of compilation of methods to support inlining.

        classMethods->second->emplace(std::make_pair(method.name(context), std::move(frame)));
    }

    for (int32_t i = 0; i < classDef.subclasses().size(); ++i) {
        auto subclass = classDef.subclasses().typedAt(i);

        subclass.setInstVarNames(
                classDef.instVarNames()
                    .copy(context, classDef.instVarNames().size() + subclass.instVarNames().size())
                    .addAll(context, subclass.instVarNames()));

        subclass.setIprototype(
                classDef.iprototype()
                    .copy(context, classDef.iprototype().size() + subclass.iprototype().size())
                    .addAll(context, subclass.iprototype()));

        if (!composeSubclassesFrom(context, subclass)) { return false; }
    }

    return true;
}

bool ClassLibrary::buildFrames(ThreadContext* /* context */) {
    return true;
}

bool ClassLibrary::cleanUp() {
    return true;
}

} // namespace hadron