#include "hadron/ClassLibrary.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Hash.hpp"
#include "hadron/Heap.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/Parser.hpp"
#include "hadron/SourceFile.hpp"

#include "schema/Common/Core/Object.hpp"
#include "schema/Common/Core/Kernel.hpp"

#include "schema/Common/Collections/Collection.hpp"
#include "schema/Common/Collections/SequenceableCollection.hpp"
#include "schema/Common/Collections/ArrayedCollection.hpp"
#include "schema/Common/Collections/Array.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

ClassLibrary::ClassLibrary(std::shared_ptr<Heap> heap, std::shared_ptr<ErrorReporter> errorReporter):
    m_heap(heap), m_errorReporter(errorReporter) {}

bool ClassLibrary::addClassFile(const std::string& classFile) {
    SourceFile sourceFile(classFile);
    if (!sourceFile.read(m_errorReporter)) {
        return false;
    }

    auto code = sourceFile.codeView();
    m_errorReporter->setCode(code.data());

    hadron::Lexer lexer(code, m_errorReporter);
    if (!lexer.lex() || !m_errorReporter->ok()) {
        SPDLOG_ERROR("Failed to lex input class file: {}\n", classFile);
        return false;
    }

    hadron::Parser parser(&lexer, m_errorReporter);
    if (!parser.parseClass() || !m_errorReporter->ok()) {
        SPDLOG_ERROR("Failed to parse input class file: {}\n", classFile);
        return false;
    }

    auto filenameSymbol = m_heap->addSymbol(classFile);
    const parse::Node* node = parser.root();
    while (node) {
        if (node->nodeType != parse::NodeType::kClass) {
            SPDLOG_ERROR("Class file didn't contain Class: {}\n", classFile);
            return false;
        }
        auto classNode = reinterpret_cast<const parse::ClassNode*>(node);
        library::Class* classDef = reinterpret_cast<library::Class*>(m_heap->allocateObject(library::kClassHash,
                sizeof(library::Class)));
        classDef->name = m_heap->addSymbol(lexer.tokens()[classNode->tokenIndex].range);
        classDef->nextclass = Slot(); // TODO
        if (classNode->superClassNameIndex) {
            classDef->superclass = m_heap->addSymbol(lexer.tokens()[classNode->superClassNameIndex.value()].range);
        } else {
            if (classDef->name == library::kObjectHash) {
                classDef->superclass = Slot();
            } else {
                classDef->superclass = library::kObjectHash;
            }
        }
        classDef->subclasses = m_heap->allocateObject(library::kArrayHash, sizeof(library::Array));
        classDef->methods = m_heap->allocateObject(library::kArrayHash, sizeof(library::Array));
        classDef->instVarNames = m_heap->allocateObject(library::kArrayHash, sizeof(library::Array));
        classDef->classVarNames = m_heap->allocateObject(library::kArrayHash, sizeof(library::Array));
        classDef->iprototype = m_heap->allocateObject(library::kArrayHash, sizeof(library::Array));
        classDef->cprototype = m_heap->allocateObject(library::kArrayHash, sizeof(library::Array));
        classDef->constNames = m_heap->allocateObject(library::kArrayHash, sizeof(library::Array));
        classDef->constValues = m_heap->allocateObject(library::kArrayHash, sizeof(library::Array));
        classDef->instanceFormat = Slot();
        classDef->instanceFlags = Slot();
        classDef->classIndex = Slot();
        classDef->classFlags = Slot();
        classDef->maxSubclassIndex = Slot();
        classDef->filenameSymbol = filenameSymbol;
        classDef->charPos = Slot(); // TODO
        classDef->classVarIndex = Slot();

        const hadron::parse::VarListNode* varList = classNode->variables.get();
        while (varList) {
            auto varHash = lexer.tokens()[varList->tokenIndex].hash;
            Slot nameArray, valueArray;
            if (varHash == hadron::kVarHash) {
                nameArray = classDef->instVarNames;
                valueArray = classDef->iprototype;
            } else if (varHash == hadron::kClassVarHash) {
                nameArray = classDef->classVarNames;
                valueArray = classDef->cprototype;
            } else if (varHash == hadron::kConstHash) {
                nameArray = classDef->constNames;
                valueArray = classDef->constValues;
            } else {
                // Parser internal error with VarListNode pointing at a token that isn't 'var', 'classvar', or 'const'.
                assert(false);
            }

            const hadron::parse::VarDefNode* varDef = varList->definitions.get();
            while (varDef) {
                library::ArrayedCollection::_ArrayAdd(nullptr, nameArray, m_heap->addSymbol(
                        lexer.tokens()[varDef->tokenIndex].range));
                if (varDef->initialValue) {
                    if (varDef->initialValue->nodeType != parse::NodeType::kLiteral) {
                        SPDLOG_ERROR("non-literal initial value in class.");
                        assert(false);
                    }
                    auto literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
                    library::ArrayedCollection::_ArrayAdd(nullptr, valueArray, literal->value);
                } else {
                    library::ArrayedCollection::_ArrayAdd(nullptr, valueArray, Slot());
                }
                varDef = reinterpret_cast<const hadron::parse::VarDefNode*>(varDef->next.get());
            }
            varList = reinterpret_cast<const hadron::parse::VarListNode*>(varList->next.get());
        }

        const hadron::parse::MethodNode* methodNode = classNode->methods.get();
        while (methodNode) {
            library::Method* method = reinterpret_cast<library::Method*>(m_heap->allocateObject(library::kMethodHash,
                sizeof(library::Method)));
            method->raw1 = Slot();
            method->raw2 = Slot();
            method->code = Slot(); // TODO - this is where the bytecode lives apparently.
            method->selectors = Slot(); // TODO: ??
            method->constants = Slot(); // Why do we need these?


            method->ownerClass = Slot(classDef);
            method->name = m_heap->addSymbol(lexer.tokens()[methodNode->tokenIndex].range);
            if (methodNode->primitiveIndex) {
                method->primitiveName = m_heap->addSymbol(lexer.tokens()[methodNode->primitiveIndex.value()].range);
            } else {
                method->primitiveName = Slot();
            }
            method->filenameSymbol = filenameSymbol;
            method->charPos = Slot(); // TODO
            library::ArrayedCollection::_ArrayAdd(nullptr, classDef->methods, Slot(method));

            methodNode = reinterpret_cast<const hadron::parse::MethodNode*>(methodNode->next.get());
        }

        node = classNode->next.get();
    }

    return true;
}


} // namespace hadron