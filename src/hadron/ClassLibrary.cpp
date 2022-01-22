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
#include "hadron/LinearBlock.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Pipeline.hpp"
#include "hadron/RegisterAllocator.hpp"
#include "hadron/Resolver.hpp"
#include "hadron/SourceFile.hpp"

#include "schema/Common/Core/Object.hpp"
#include "schema/Common/Core/Kernel.hpp"

#include "schema/Common/Collections/Collection.hpp"
#include "schema/Common/Collections/SequenceableCollection.hpp"
#include "schema/Common/Collections/ArrayedCollection.hpp"
#include "schema/Common/Collections/Array.hpp"

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
// Second Pass: Starting from Object, do breadth-first or pre-order traversal through Object heirarchy. Concatenate
//   existing intVarNames, iprototype elements into array containing all superclass data. Compile individual methods
//   of each Class now that we know the entire context of the subclass heirarchy.

namespace hadron {

ClassLibrary::ClassLibrary(std::shared_ptr<ErrorReporter> errorReporter):
    m_errorReporter(errorReporter) {}

bool ClassLibrary::addClassFile(ThreadContext* context, const std::string& classFile) {
    SourceFile sourceFile(classFile);
    if (!sourceFile.read(m_errorReporter)) {
        return false;
    }

    auto code = sourceFile.codeView();
    m_errorReporter->setCode(code.data());

    Lexer lexer(code, m_errorReporter);
    if (!lexer.lex() || !m_errorReporter->ok()) {
        SPDLOG_ERROR("Failed to lex input class file: {}\n", classFile);
        return false;
    }

    Parser parser(&lexer, m_errorReporter);
    if (!parser.parseClass() || !m_errorReporter->ok()) {
        SPDLOG_ERROR("Failed to parse input class file: {}\n", classFile);
        return false;
    }

    auto filenameSymbol = context->heap->addSymbol(classFile);
    const parse::Node* node = parser.root();
    while (node) {
        if (node->nodeType != parse::NodeType::kClass) {
            SPDLOG_ERROR("Class file didn't contain Class: {}\n", classFile);
            return false;
        }
        auto classNode = reinterpret_cast<const parse::ClassNode*>(node);
        auto classSlot = buildClass(context, filenameSymbol, classNode, &lexer);
        if (classSlot.isNil()) {
            SPDLOG_ERROR("Error compiling Class {} in {}, skipping\n",
                    lexer.tokens()[classNode->tokenIndex].range, classFile);
            node = classNode->next.get();
            continue;
        }
        library::Class* classDef = reinterpret_cast<library::Class*>(classSlot.getPointer());
        assert(classDef->_className == library::kClassHash);
        m_classTable[classDef->name.getHash()] = classDef;
        node = classNode->next.get();
    }

    return true;
}

Slot ClassLibrary::buildClass(ThreadContext* context, Slot filenameSymbol, const parse::ClassNode* classNode,
        const Lexer* lexer) {
    // Compiling a class actually involves generating an instance of the Class object, and then generating an
    // instance of a Meta_ClassName object derived from the Class object, which is where class methods typically go.
    library::Class* classDef = reinterpret_cast<library::Class*>(context->heap->allocateObject(
            library::kClassHash, sizeof(library::Class)));
    library::Class* metaClassDef = reinterpret_cast<library::Class*>(context->heap->allocateObject(
            library::kClassHash, sizeof(library::Class)));

    SPDLOG_INFO("Class Library compiling class {}", lexer->tokens()[classNode->tokenIndex].range);
    classDef->name = context->heap->addSymbol(lexer->tokens()[classNode->tokenIndex].range);
    metaClassDef->name = context->heap->addSymbol(fmt::format("Meta_{}", lexer->tokens()[classNode->tokenIndex].range));

    classDef->nextclass = Slot::makeNil(); // TODO
    metaClassDef->nextclass = Slot::makeNil();

    if (classNode->superClassNameIndex) {
        classDef->superclass = context->heap->addSymbol(
                lexer->tokens()[classNode->superClassNameIndex.value()].range);
        metaClassDef->name = context->heap->addSymbol(fmt::format("Meta_{}",
                lexer->tokens()[classNode->superClassNameIndex.value()].range));
    } else {
        if (classDef->name.getHash() == library::kObjectHash) {
            classDef->superclass = Slot::makeNil();
            // The superclass of 'Meta_Object' is 'Class'.
            metaClassDef->superclass = Slot::makeHash(library::kClassHash);
        } else {
            classDef->superclass = Slot::makeHash(library::kObjectHash);
            metaClassDef->superclass = Slot::makeHash(library::kMetaObjectHash);
        }
    }

    classDef->subclasses = Slot::makePointer(context->heap->allocateObject(library::kArrayHash,
            sizeof(library::Array)));
    classDef->subclasses = Slot::makePointer(context->heap->allocateObject(library::kArrayHash,
            sizeof(library::Array)));

    classDef->methods = Slot::makePointer(context->heap->allocateObject(library::kArrayHash, sizeof(library::Array)));
    classDef->instVarNames = Slot::makePointer(context->heap->allocateObject(library::kSymbolArrayHash,
            sizeof(library::SymbolArray)));
    classDef->classVarNames = Slot::makePointer(context->heap->allocateObject(library::kSymbolArrayHash,
            sizeof(library::SymbolArray)));
    classDef->iprototype = Slot::makePointer(context->heap->allocateObject(library::kArrayHash,
            sizeof(library::Array)));
    classDef->cprototype = Slot::makePointer(context->heap->allocateObject(library::kArrayHash,
            sizeof(library::Array)));
    classDef->constNames = Slot::makePointer(context->heap->allocateObject(library::kSymbolArrayHash,
            sizeof(library::SymbolArray)));
    classDef->constValues = Slot::makePointer(context->heap->allocateObject(library::kArrayHash,
            sizeof(library::Array)));
    classDef->instanceFormat = Slot::makeNil();
    classDef->instanceFlags = Slot::makeNil();
    classDef->classIndex = Slot::makeNil();
    classDef->classFlags = Slot::makeNil();
    classDef->maxSubclassIndex = Slot::makeNil();
    classDef->filenameSymbol = filenameSymbol;
    classDef->charPos = Slot::makeNil(); // TODO
    classDef->classVarIndex = Slot::makeNil();
    Slot classSlot = Slot::makePointer(classDef);

    const parse::VarListNode* varList = classNode->variables.get();
    while (varList) {
        auto varHash = lexer->tokens()[varList->tokenIndex].hash;
        Slot nameArray = Slot::makeNil();
        Slot valueArray = Slot::makeNil();
        if (varHash == kVarHash) {
            nameArray = classDef->instVarNames;
            valueArray = classDef->iprototype;
        } else if (varHash == kClassVarHash) {
            nameArray = classDef->classVarNames;
            valueArray = classDef->cprototype;
        } else if (varHash == kConstHash) {
            nameArray = classDef->constNames;
            valueArray = classDef->constValues;
        } else {
            // Parser internal error with VarListNode pointing at a token that isn't 'var', 'classvar', or 'const'.
            assert(false);
        }

        const parse::VarDefNode* varDef = varList->definitions.get();
        while (varDef) {
            library::ArrayedCollection::_ArrayAdd(context, nameArray, context->heap->addSymbol(
                    lexer->tokens()[varDef->tokenIndex].range));
            if (varDef->initialValue) {
                if (varDef->initialValue->nodeType != parse::NodeType::kLiteral) {
                    SPDLOG_ERROR("non-literal initial value in class.");
                    assert(false);
                }
                auto literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
                library::ArrayedCollection::_ArrayAdd(context, valueArray, literal->value);
            } else {
                library::ArrayedCollection::_ArrayAdd(context, valueArray, Slot::makeNil());
            }
            varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        }
        varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
    }

    const parse::MethodNode* methodNode = classNode->methods.get();
    while (methodNode) {
        Slot methodSlot = buildMethod(context, classDef, methodNode, lexer);
        if (methodSlot.isNil()) {
            SPDLOG_ERROR("Error compiling method {} in class {}, skipping.",
                    lexer->tokens()[classNode->tokenIndex].range);
            methodNode = reinterpret_cast<const parse::MethodNode*>(methodNode->next.get());
            continue;
        }
        assert(methodSlot.isPointer());
        library::Method* method = reinterpret_cast<library::Method*>(methodSlot.getPointer());
        assert(method->_className == library::kMethodHash);
        library::ArrayedCollection::_ArrayAdd(context, classDef->methods, methodSlot);
        methodNode = reinterpret_cast<const parse::MethodNode*>(methodNode->next.get());
    }

    return classSlot;
}

} // namespace hadron