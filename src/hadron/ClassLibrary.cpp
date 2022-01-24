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
#include "hadron/ThreadContext.hpp"
#include "internal/FileSystem.hpp"

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
//   existing intVarNames, iprototype elements into array containing all superclass data.
//
// Third Pass: Now that the full pedigree and member variables names of each object is known, we compile the individual
//   methods.

namespace {
static const std::array<const char*, 11> kClassFileAllowList{
    "AbstractFunction.sc"
    "Array.sc"
    "ArrayedCollection.sc"
    "Boolean.sc"
    "Char.sc"
    "Collection.sc"
    "Dictionary.sc"
    "Float.sc"
    "Function.sc"
    "Integer.sc"
    "Kernel.sc"
    "Magnitude.sc"
    "Nil.sc"
    "Number.sc"
    "Object.sc"
    "SequenceableCollection.sc"
    "Set.sc"
    "SimpleNumber.sc"
    "Symbol.sc"
};
}

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
    if (m_classArray) {
        context->heap->removeFromRootSet(Slot::makePointer(m_classArray));
    }
    m_classFiles.clear();
    m_cachedSubclassArrays.clear();

    m_classArray = reinterpret_cast<library::Array*>(context->heap->allocateObject(library::kArrayHash,
            sizeof(library::Array)));
    context->heap->addToRootSet(Slot::makePointer(m_classArray));
    return true;
}

bool ClassLibrary::scanFiles(ThreadContext* context) {
    for (const auto& classLibPath : m_libraryPaths) {
        for (auto& entry : fs::recursive_directory_iterator(classLibPath)) {
            const auto& path = fs::absolute(entry.path());
            if (!fs::is_regular_file(path) || path.extension() != ".sc")
                continue;
            // For now we have an allowlist in place, to control which SC class files are parsed, and we lazily do an
            // O(n) search in the array for each one.
            bool classFileAllowed = false;
            for (size_t i = 0; i < kClassFileAllowList.size(); ++i) {
                if (path.filename().compare(kClassFileAllowList[i]) == 0) {
                    classFileAllowed = true;
                    break;
                }
            }
            if (!classFileAllowed) {
                // SPDLOG_WARN("Skipping compilation of {}, not in class file allow list.", path.c_str());
                continue;
            }

            SPDLOG_INFO("Class Library scanning '{}'", path.c_str());

            auto sourceFile = std::make_unique<SourceFile>(path.string());
            if (!sourceFile->read(m_errorReporter)) { return false; }

            auto lexer = std::make_unique<Lexer>(sourceFile->codeView(), m_errorReporter);
            if (!lexer->lex()) { return false; }

            auto parser = std::make_unique<Parser>(lexer.get(), m_errorReporter);
            if (!parser->parseClass()) { return false; }

            auto filename = context->heap->addSymbol(path.string()).getHash();
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

bool ClassLibrary::scanClass(ThreadContext* context, Hash filename, int32_t charPos, const parse::ClassNode* classNode,
        const Lexer* lexer) {
    // Compiling a class actually involves generating an instance of the Class object, and then generating an
    // instance of a Meta_ClassName object derived from the Class object, which is where class methods go.
    library::Class* classDef = reinterpret_cast<library::Class*>(context->heap->allocateObject(
            library::kClassHash, sizeof(library::Class)));
    library::Class* metaClassDef = reinterpret_cast<library::Class*>(context->heap->allocateObject(
            library::kClassHash, sizeof(library::Class)));

    SPDLOG_INFO("Class Library compiling class {}", lexer->tokens()[classNode->tokenIndex].range);
    classDef->name = context->heap->addSymbol(lexer->tokens()[classNode->tokenIndex].range);
    metaClassDef->name = context->heap->addSymbol(fmt::format("Meta_{}", lexer->tokens()[classNode->tokenIndex].range));

    classDef->nextclass = Slot::makeNil();
    metaClassDef->nextclass = Slot::makeNil();

    if (classNode->superClassNameIndex) {
        classDef->superclass = context->heap->addSymbol(
                lexer->tokens()[classNode->superClassNameIndex.value()].range);
        metaClassDef->superclass = context->heap->addSymbol(fmt::format("Meta_{}",
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

    // Find and add the class and metaClass to the appropriate superclass object subclasses list.
    addToSubclassArray(context, classDef);
    addToSubclassArray(context, metaClassDef);

    classDef->subclasses = getSubclassArray(classDef);
    metaClassDef->subclasses = getSubclassArray(metaClassDef);

    classDef->methods = Slot::makeNil();
    classDef->instVarNames = Slot::makeNil();
    classDef->classVarNames = Slot::makeNil();
    classDef->iprototype = Slot::makeNil();
    classDef->cprototype = Slot::makeNil();
    classDef->constNames = Slot::makeNil();
    classDef->constValues = Slot::makeNil();
    classDef->instanceFormat = Slot::makeNil();
    classDef->instanceFlags = Slot::makeNil();
    classDef->classIndex = Slot::makeNil();
    classDef->classFlags = Slot::makeNil();
    classDef->maxSubclassIndex = Slot::makeNil();
    classDef->filenameSymbol = Slot::makeHash(filename);
    classDef->charPos = Slot::makeInt32(charPos);
    classDef->classVarIndex = Slot::makeNil();

    metaClassDef->methods = Slot::makeNil();
    metaClassDef->instVarNames = Slot::makeNil();
    metaClassDef->classVarNames = Slot::makeNil();
    metaClassDef->iprototype = Slot::makeNil();
    metaClassDef->cprototype = Slot::makeNil();
    metaClassDef->constNames = Slot::makeNil();
    metaClassDef->constValues = Slot::makeNil();
    metaClassDef->instanceFormat = Slot::makeNil();
    metaClassDef->instanceFlags = Slot::makeNil();
    metaClassDef->classIndex = Slot::makeNil();
    metaClassDef->classFlags = Slot::makeNil();
    metaClassDef->maxSubclassIndex = Slot::makeNil();
    metaClassDef->filenameSymbol = Slot::makeHash(filename);
    metaClassDef->charPos = Slot::makeInt32(charPos);
    metaClassDef->classVarIndex = Slot::makeNil();

    const parse::VarListNode* varList = classNode->variables.get();
    while (varList) {
        auto varHash = lexer->tokens()[varList->tokenIndex].hash;
        Slot* nameArray = nullptr;
        Slot* valueArray = nullptr;
        if (varHash == kVarHash) {
            nameArray = &classDef->instVarNames;
            valueArray = &classDef->iprototype;
        } else if (varHash == kClassVarHash) {
            nameArray = &classDef->classVarNames;
            valueArray = &classDef->cprototype;
        } else if (varHash == kConstHash) {
            nameArray = &classDef->constNames;
            valueArray = &classDef->constValues;
        }
        // Internal error with VarListNode pointing at a token that isn't 'var', 'classvar', or 'const'.
        assert(nameArray);
        assert(valueArray);
        if (nameArray->isNil()) {
            *nameArray = Slot::makePointer(context->heap->allocateObject(library::kSymbolArrayHash,
                    sizeof(library::SymbolArray)));
        }
        if (valueArray->isNil()) {
            *valueArray = Slot::makePointer(context->heap->allocateObject(library::kArrayHash,
                    sizeof(library::Array)));
        }

        const parse::VarDefNode* varDef = varList->definitions.get();
        while (varDef) {
            *nameArray = library::ArrayedCollection::_ArrayAdd(context, *nameArray, context->heap->addSymbol(
                    lexer->tokens()[varDef->tokenIndex].range));
            if (varDef->initialValue) {
                if (varDef->initialValue->nodeType != parse::NodeType::kLiteral) {
                    SPDLOG_ERROR("non-literal initial value in class.");
                    assert(false);
                }
                auto literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
                *valueArray = library::ArrayedCollection::_ArrayAdd(context, *valueArray, literal->value);
            } else {
                *valueArray = library::ArrayedCollection::_ArrayAdd(context, *valueArray, Slot::makeNil());
            }
            varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
        }
        varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
    }

    m_classMap.emplace(std::make_pair(classDef->name.getHash(), classDef));
    m_classMap.emplace(std::make_pair(metaClassDef->name.getHash(), metaClassDef));
    appendToClassArray(context, classDef);
    appendToClassArray(context, metaClassDef);

    return true;
}

// As we encounter the classes in undefined order during the initial scan, we build the subclassses array in each class
// object by adding to it if the class already exists, or by caching an array in m_cachedSubclassArrays if the class
// doesn't exist yet.
void ClassLibrary::addToSubclassArray(ThreadContext* context, const library::Class* subclass) {
    auto superclassHash = subclass->superclass.getHash();
    auto superclassIter = m_classMap.find(superclassHash);
    if (superclassIter != m_classMap.end()) {
        if (superclassIter->second->subclasses.isNil()) {
            superclassIter->second->subclasses = Slot::makePointer(context->heap->allocateObject(library::kArrayHash,
                    sizeof(library::Array)));
        }
        superclassIter->second->subclasses = library::Array::_ArrayAdd(context, superclassIter->second->subclasses,
                Slot::makePointer(subclass));
        return;
    }
    auto arrayIter = m_cachedSubclassArrays.find(superclassHash);
    if (arrayIter != m_cachedSubclassArrays.end()) {
        arrayIter->second = reinterpret_cast<library::Array*>(library::Array::_ArrayAdd(context,
                Slot::makePointer(arrayIter->second), Slot::makePointer(subclass)).getPointer());
        return;
    }
    auto subclassArray = reinterpret_cast<library::Array*>(context->heap->allocateObject(library::kArrayHash,
            sizeof(library::Array)));
    m_cachedSubclassArrays.emplace(std::make_pair(superclassHash, reinterpret_cast<library::Array*>(
            library::Array::_ArrayAdd(context, Slot::makePointer(subclassArray),
            Slot::makePointer(subclass)).getPointer())));
}

Slot ClassLibrary::getSubclassArray(const library::Class* superclass) {
    // First look in the cached arrays, erase and return if found.
    auto arrayIter = m_cachedSubclassArrays.find(superclass->name.getHash());
    if (arrayIter != m_cachedSubclassArrays.end()) {
        auto cachedArray = arrayIter->second;
        m_cachedSubclassArrays.erase(arrayIter);
        return Slot::makePointer(cachedArray);
    }

    return Slot::makeNil();
}

void ClassLibrary::appendToClassArray(ThreadContext* context, library::Class* classDef) {
    auto arraySize = library::Array::_BasicSize(context, Slot::makePointer(m_classArray)).getInt32();
    if (arraySize > 0) {
        auto prevClassSlot = library::Array::_BasicAt(context, Slot::makePointer(m_classArray),
                Slot::makeInt32(arraySize - 1));
        auto prevClass = reinterpret_cast<library::Class*>(prevClassSlot.getPointer());
        prevClass->nextclass = Slot::makePointer(classDef);
    }

    classDef->classIndex = Slot::makeInt32(arraySize);

    auto arraySlot = library::Array::_ArrayAdd(context, Slot::makePointer(m_classArray), Slot::makePointer(classDef));
    if (arraySlot != Slot::makePointer(m_classArray)) {
        context->heap->addToRootSet(arraySlot);
        context->heap->removeFromRootSet(Slot::makePointer(m_classArray));
        m_classArray = reinterpret_cast<library::Array*>(arraySlot.getPointer());
    }
}

bool ClassLibrary::finalizeHeirarchy(ThreadContext* context) {
    auto objectIter = m_classMap.find(library::kObjectHash);
    if (objectIter == m_classMap.end()) { assert(false); return false; }

    // We start at the root of the class heirarchy with Object.
    auto objectClassDef = objectIter->second;
    composeSubclassesFrom(context, objectClassDef);

    return true;
}

void ClassLibrary::composeSubclassesFrom(ThreadContext* context, library::Class* classDef) {
    auto subclassesSize = library::Array::_BasicSize(context, classDef->subclasses).getInt32();
    if (subclassesSize > 0) {
        classDef->maxSubclassIndex = Slot::makeInt32(subclassesSize - 1);
    } else {
        classDef->maxSubclassIndex = Slot::makeNil();
    }

    for (int32_t i = 0; i < subclassesSize; ++i) {
        auto subclassSlot = library::Array::_BasicAt(context, classDef->subclasses, Slot::makeInt32(i));
        auto subclass = reinterpret_cast<library::Class*>(subclassSlot.getPointer());
        assert(subclass->superclass == Slot::makePointer(classDef));
        subclass->instVarNames = concatenateArrays(context, classDef->instVarNames, subclass->instVarNames);
        subclass->classVarNames = concatenateArrays(context, classDef->classVarNames, subclass->classVarNames);
        subclass->iprototype = concatenateArrays(context, classDef->iprototype, subclass->iprototype);
        subclass->cprototype = concatenateArrays(context, classDef->cprototype, subclass->cprototype);
        subclass->constNames = concatenateArrays(context, classDef->constNames, subclass->constNames);
        subclass->constValues = concatenateArrays(context, classDef->constNames, subclass->constValues);
    }
}

// TODO: it's a code smell that there's this pure utility function for manipulating arrays in the ClassLibrary class.
// The sclang Nil class has some nice overloads for adding and concatenating arrays, but that happens on the sclang
// side, so by the time we get to the primitives they are very much assuming they are method calls on objects in the
// class heirarchy. I considered adding similar convenience code in the primitives, but this makes a big assumption that
// the return type will be 'Array', when some of these arrays are 'SymbolArray' objects. Hopefully after the
// ClassLibrary is compiled manipulation of SC data structres can happen *in the sc language,* thus justifying this
// bootstrap code smell. :)
Slot ClassLibrary::concatenateArrays(ThreadContext* context, Slot prefix, Slot suffix) {
    if (prefix.isNil()) { return suffix; }

    auto prefixSize = library::Array::_BasicSize(context, prefix).getInt32();
    assert(prefixSize > 0);
    Hash prefixType = prefix.getPointer()->_className;

    int32_t suffixSize = 0;
    if (suffix.isPointer()) {
        suffixSize = library::Array::_BasicSize(context, suffix).getInt32();
        assert(prefixType == suffix.getPointer()->_className);
    }

    auto array = context->heap->allocateObject(prefixType, sizeof(ObjectHeader) +
            ((prefixSize + suffixSize) * sizeof(Slot)));
    std::memcpy(reinterpret_cast<uint8_t*>(array) + sizeof(ObjectHeader),
                reinterpret_cast<uint8_t*>(prefix.getPointer()) + sizeof(ObjectHeader),
                sizeof(Slot) * prefixSize);
    if (suffixSize > 0) {
        std::memcpy(reinterpret_cast<uint8_t*>(array) + sizeof(ObjectHeader) + (sizeof(Slot) * prefixSize),
                reinterpret_cast<uint8_t*>(suffix.getPointer()) + sizeof(ObjectHeader),
                sizeof(Slot) * suffixSize);
    }

    return Slot::makePointer(array);
}

bool ClassLibrary::compileMethods(ThreadContext* /* context */) {
    return false;
}

bool ClassLibrary::cleanUp() {
    m_classFiles.clear();
    m_cachedSubclassArrays.clear();
    return true;
}


} // namespace hadron