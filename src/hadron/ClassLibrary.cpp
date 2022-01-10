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
        m_classTable[classDef->name] = classSlot;
        node = classNode->next.get();
    }

    return true;
}

hadron::Slot ClassLibrary::buildClass(ThreadContext* context, Slot filenameSymbol,
        const hadron::parse::ClassNode* classNode, const Lexer* lexer) {
    library::Class* classDef = reinterpret_cast<library::Class*>(context->heap->allocateObject(
            library::kClassHash, sizeof(library::Class)));

    SPDLOG_INFO("Class Library compiling class {}", lexer->tokens()[classNode->tokenIndex].range);
    classDef->name = context->heap->addSymbol(lexer->tokens()[classNode->tokenIndex].range);
    classDef->nextclass = Slot(); // TODO
    if (classNode->superClassNameIndex) {
        classDef->superclass = context->heap->addSymbol(
                lexer->tokens()[classNode->superClassNameIndex.value()].range);
    } else {
        if (classDef->name == library::kObjectHash) {
            classDef->superclass = Slot();
        } else {
            classDef->superclass = library::kObjectHash;
        }
    }
    classDef->subclasses = context->heap->allocateObject(library::kArrayHash, sizeof(library::Array));
    classDef->methods = context->heap->allocateObject(library::kArrayHash, sizeof(library::Array));
    classDef->instVarNames = context->heap->allocateObject(library::kSymbolArrayHash, sizeof(library::SymbolArray));
    classDef->classVarNames = context->heap->allocateObject(library::kSymbolArrayHash,
            sizeof(library::SymbolArray));
    classDef->iprototype = context->heap->allocateObject(library::kArrayHash, sizeof(library::Array));
    classDef->cprototype = context->heap->allocateObject(library::kArrayHash, sizeof(library::Array));
    classDef->constNames = context->heap->allocateObject(library::kSymbolArrayHash, sizeof(library::SymbolArray));
    classDef->constValues = context->heap->allocateObject(library::kArrayHash, sizeof(library::Array));
    classDef->instanceFormat = Slot();
    classDef->instanceFlags = Slot();
    classDef->classIndex = Slot();
    classDef->classFlags = Slot();
    classDef->maxSubclassIndex = Slot();
    classDef->filenameSymbol = filenameSymbol;
    classDef->charPos = Slot(); // TODO
    classDef->classVarIndex = Slot();
    Slot classSlot = Slot(classDef);

    const hadron::parse::VarListNode* varList = classNode->variables.get();
    while (varList) {
        auto varHash = lexer->tokens()[varList->tokenIndex].hash;
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
                library::ArrayedCollection::_ArrayAdd(context, valueArray, Slot());
            }
            varDef = reinterpret_cast<const hadron::parse::VarDefNode*>(varDef->next.get());
        }
        varList = reinterpret_cast<const hadron::parse::VarListNode*>(varList->next.get());
    }

    const hadron::parse::MethodNode* methodNode = classNode->methods.get();
    while (methodNode) {
        Slot methodSlot = buildMethod(context, classDef, methodNode, lexer);
        if (methodSlot.isNil()) {
            SPDLOG_ERROR("Error compiling method {} in class {}, skipping.",
                    lexer->tokens()[classNode->tokenIndex].range);
            methodNode = reinterpret_cast<const hadron::parse::MethodNode*>(methodNode->next.get());
            continue;
        }
        assert(methodSlot.isPointer());
        library::Method* method = reinterpret_cast<library::Method*>(methodSlot.getPointer());
        assert(method->_className == library::kMethodHash);
        library::ArrayedCollection::_ArrayAdd(context, classDef->methods, methodSlot);
        methodNode = reinterpret_cast<const hadron::parse::MethodNode*>(methodNode->next.get());
    }

    return classSlot;
}

hadron::Slot ClassLibrary::buildMethod(ThreadContext* context, library::Class* classDef,
        const hadron::parse::MethodNode* methodNode, const Lexer* lexer) {
    SPDLOG_INFO("Class Library compiling method {}", lexer->tokens()[methodNode->tokenIndex].range);
    library::Method* method = reinterpret_cast<library::Method*>(context->heap->allocateObject(library::kMethodHash,
            sizeof(library::Method)));
    method->raw1 = Slot();
    method->raw2 = Slot();

    // JIT compile the bytecode.
    BlockBuilder blockBuilder(lexer, m_errorReporter);
    auto frame = blockBuilder.buildFrame(context, methodNode->body.get());
    BlockSerializer blockSerializer;
    auto linearBlock = blockSerializer.serialize(std::move(frame));
    LifetimeAnalyzer lifetimeAnalyzer;
    lifetimeAnalyzer.buildLifetimes(linearBlock.get());
    RegisterAllocator registerAllocator(kNumberOfPhysicalRegisters);
    registerAllocator.allocateRegisters(linearBlock.get());
    Resolver resolver;
    resolver.resolve(linearBlock.get());

    // Estimate output size of JIT buffer as 16 bytes per HIR instruction. This is a SWAG and could perhaps be adjusted
    // or refined, but gives us the size class of the JIT buffer allocation.
    size_t jitSize = (linearBlock->instructions.size() * 16) + sizeof(library::Int8Array);
    size_t jitAllocationSize = 0;
    library::Int8Array* jitArray = context->heap->allocateJIT(jitSize, jitAllocationSize);
    LighteningJIT jit;
    jit.begin(reinterpret_cast<uint8_t*>(jitArray) + sizeof(library::Int8Array),
            jitAllocationSize - sizeof(library::Int8Array));
    Emitter emitter;
    emitter.emit(linearBlock.get(), &jit);

    // Compute the actual size of the jit and update the size of the array to match.
    size_t finalSize = 0;
    jit.end(&finalSize);
    assert(finalSize + sizeof(library::Int8Array) < jitAllocationSize);
    jitArray->_sizeInBytes = finalSize + sizeof(library::Int8Array);
    method->code = Slot(jitArray);

    method->selectors = Slot(); // TODO: ??
    method->constants = Slot(); // TODO: ??
    method->prototypeFrame = context->heap->allocateObject(library::kArrayHash, sizeof(library::Array));
    method->context = Slot(); // TODO
    method->argNames = context->heap->allocateObject(library::kSymbolArrayHash, sizeof(library::SymbolArray));
    method->varNames = context->heap->allocateObject(library::kSymbolArrayHash, sizeof(library::SymbolArray));
    method->sourceCode = Slot(); // TODO
    method->ownerClass = Slot(classDef);
    method->name = context->heap->addSymbol(lexer->tokens()[methodNode->tokenIndex].range);
    if (methodNode->primitiveIndex) {
        method->primitiveName = context->heap->addSymbol(lexer->tokens()[methodNode->primitiveIndex.value()].range);
    } else {
        method->primitiveName = Slot();
    }
    method->filenameSymbol = classDef->filenameSymbol;
    method->charPos = Slot(); // TODO

    return Slot(method);
}

} // namespace hadron