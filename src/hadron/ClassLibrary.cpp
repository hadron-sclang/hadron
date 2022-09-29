#include "hadron/ClassLibrary.hpp"

#include "hadron/ASTBuilder.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/Generator.hpp"
#include "hadron/Hash.hpp"
#include "hadron/Heap.hpp"
#include "hadron/internal/FileSystem.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/Parser.hpp"
#include "hadron/SourceFile.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"

#include "spdlog/spdlog.h"

#include <cassert>

/*

Heirarchy
---------

Object
|
v
Class ---------\
|              |
v              v
Meta_Object    A
|              |
v              v
Meta_A         B
|              |
v              v
Meta_B         C
|
v
Meta_C

Instances
---------

A <== Meta_A <== Class <== Meta_Class
                  ||           ^
                  \\==========//

A is an instance of Meta_A
Meta_A is an instance of Class
Class is an instance of Meta_Class
Meta_Class is an instance of Class

*/

namespace hadron {

ClassLibrary::ClassLibrary(): m_numberOfClassVariables(0) { }

void ClassLibrary::bootstrapLibrary(ThreadContext* context) {
    {
#include "hadron/ClassLibraryBootstrap.cpp"
    }

    // Construct an empty Method object for the Interpreter to serve as a compilation context for interpreted code.
    auto interpreterClass = findClassNamed(context->symbolTable->interpreterSymbol());
    assert(interpreterClass);
    m_functionCompileContext = library::Method::alloc(context);
    m_functionCompileContext.initToNil();
    m_functionCompileContext.setOwnerClass(interpreterClass);
    m_functionCompileContext.setName(context->symbolTable->functionCompileContextSymbol());
    interpreterClass.setMethods(interpreterClass.methods().typedAdd(context, m_functionCompileContext));

    // Make the connection between "Class" and "Object" or the tree won't be specified correctly
    auto objectClass = findClassNamed(context->symbolTable->objectSymbol());
    assert(objectClass);
    auto classClass = findClassNamed(context->symbolTable->classSymbol());
    assert(classClass);
    objectClass.setSubclasses(objectClass.subclasses().typedAdd(context, classClass));
    classClass.setSuperclass(context->symbolTable->objectSymbol());
    // Add connection between "Meta_Object" and "Class" also to support tree
    auto metaObjectClass = findOrInitClass(context, library::Symbol::fromView(context, "Meta_Object"));
    assert(metaObjectClass);
    classClass.setSubclasses(classClass.subclasses().typedAdd(context, metaObjectClass));
    metaObjectClass.setSuperclass(context->symbolTable->classSymbol());
}

bool ClassLibrary::scanString(ThreadContext* context, std::string_view input, library::Symbol filename) {
    auto lexer = Lexer(input);
    if (!lexer.lex()) {
        return false;
    }

    auto parser = Parser(&lexer);
    if (!parser.parseClass(context)) {
        return false;
    }

    library::Node node = parser.root();
    while (node) {
        if (node.className() != library::ClassNode::nameHash()
            && node.className() != library::ClassExtNode::nameHash()) {
            SPDLOG_ERROR("Expecting either Class or Class Extensions only at top level in class file {}",
                         filename.view(context));
            return false;
        }

        library::Symbol className = node.token().snippet(context);
        library::Class classDef = findOrInitClass(context, className);

        library::Symbol metaClassName =
            library::Symbol::fromView(context, fmt::format("Meta_{}", className.view(context)));
        library::Class metaClassDef = findOrInitClass(context, metaClassName);

        auto methodNode = library::MethodNode();

        if (node.className() == library::ClassNode::nameHash()) {
            const auto classNode = library::ClassNode(node.slot());

            int32_t charPos = classNode.token().offset();
            classDef.setFilenameSymbol(filename);
            classDef.setCharPos(charPos);
            metaClassDef.setFilenameSymbol(filename);
            metaClassDef.setCharPos(charPos);

            if (!scanClass(context, classDef, metaClassDef, classNode)) {
                return false;
            }

            methodNode = classNode.methods();
        } else {
            assert(node.className() == library::ClassExtNode::nameHash());
            const auto classExtNode = library::ClassExtNode(node.slot());
            methodNode = classExtNode.methods();
        }

        while (methodNode) {
            library::Symbol methodName = methodNode.token().snippet(context);

            if (className == context->symbolTable->interpreterSymbol()
                && methodName == context->symbolTable->functionCompileContextSymbol()) {
                // Avoid re-defining the interpreter compile context special method.
                methodNode = library::MethodNode(methodNode.next().slot());
                continue;
            }

            library::Method method = library::Method::alloc(context);
            method.initToNil();

            library::Class methodClassDef = methodNode.isClassMethod() ? metaClassDef : classDef;
            method.setOwnerClass(methodClassDef);

            library::MethodArray methodArray = methodClassDef.methods();
            methodArray = methodArray.typedAdd(context, method);
            methodClassDef.setMethods(methodArray);

            method.setName(methodName);

            if (methodNode.primitiveToken()) {
                library::Symbol primitiveName = methodNode.primitiveToken().snippet(context);
                method.setPrimitiveName(primitiveName);
            }

            // Build the AST from the MethodNode block.
            ASTBuilder astBuilder;
            auto ast = astBuilder.buildBlock(context, methodNode.body());
            if (ast.isNil()) {
                return false;
            }

            // Attach argument names from AST to the method definition.
            method.setArgNames(ast.argumentNames());

            auto methodIter = m_methodASTs.find(methodClassDef.name(context));
            assert(methodIter != m_methodASTs.end());
            methodIter->second->emplace(std::make_pair(methodName, std::move(ast)));

            method.setFilenameSymbol(filename);

            method.setCharPos(methodNode.token().offset());

            methodNode = library::MethodNode(methodNode.next().slot());
        }

        node = node.next();
    }

    return true;
}

bool ClassLibrary::finalizeLibrary(ThreadContext* context) {
    if (!finalizeHeirarchy(context)) {
        return false;
    }
    if (!materializeFrames(context)) {
        return false;
    }
    return cleanUp();
}

library::Class ClassLibrary::findClassNamed(library::Symbol name) const {
    if (!name) {
        return library::Class();
    }
    auto classIter = m_classMap.find(name);
    if (classIter == m_classMap.end()) {
        return library::Class();
    }
    return library::Class::wrapUnsafe(classIter->second);
}

// static
uint64_t ClassLibrary::dispatch(ThreadContext* context, Hash selectorHash, int numArgs,
        int numKeyArgs, schema::FramePrivateSchema* callerFrame, Slot* stackPointer) {
    auto selector = library::Symbol(context, selectorHash);

    // Should be at least 1 arg, the `this` arg, load it.
    assert(numArgs >= 1);
    Slot targetSlot = *stackPointer;

    // TODO: non-object routing (e.g. Integer)
    assert(targetSlot.isPointer());
    auto target = library::ObjectBase::wrapUnsafe(targetSlot);
    auto className = library::Symbol(context, target.className());
    auto classDef = context->classLibrary->findClassNamed(className);

    auto method = library::Method();
    while (classDef && !method) {
        for (int32_t i = 0; i < classDef.methods().size(); ++i) {
            if (classDef.methods().typedAt(i).name(context) == selector) {
                method = classDef.methods().typedAt(i);
                break;
            }
        }
        classDef = context->classLibrary->findClassNamed(classDef.superclass(context));
    }
    if (!method) {
        SPDLOG_ERROR("Failed to find method {} in class {}", selector.view(context),
                classDef.name(context).view(context));
        return Slot::makeNil().asBits();
    }

    int32_t numUsableArgs = std::min(method.argNames().size(), numArgs);

    // Init frame with inorder arguments.
    auto calleeFrame = library::Frame::alloc(context, method.prototypeFrame().size());
    calleeFrame.setMethod(method);
    calleeFrame.setCaller(library::Frame(callerFrame));
    calleeFrame.setContext(calleeFrame);
    calleeFrame.setHomeContext(calleeFrame);
    calleeFrame.setArg0(targetSlot);
    std::memcpy(reinterpret_cast<int8_t*>(calleeFrame.instance()) + sizeof(schema::FramePrivateSchema),
            reinterpret_cast<int8_t*>(stackPointer) + kSlotSize, numUsableArgs - 1);

    assert(method.prototypeFrame().size() >= numUsableArgs);

    // Init any uninitialized inorder args and all variables with prototype frame.
    std::memcpy(reinterpret_cast<int8_t*>(calleeFrame.instance()) + sizeof(schema::FramePrivateSchema) +
            (numUsableArgs * kSlotSize), reinterpret_cast<int8_t*>(method.prototypeFrame().start()) + (numUsableArgs * kSlotSize),
            (method.prototypeFrame().size() - numUsableArgs) * kSlotSize);

    // Process keyword arguments.
    if (numKeyArgs) {
        Slot* keyArg = stackPointer + numArgs;
        for (int32_t i = 0; i < numKeyArgs; ++i) {
            auto keyName = library::Symbol(context, *keyArg);
            ++keyArg;
            auto keyValue = *keyArg;
            ++keyArg;

            for (int32_t j = 0; j < method.argNames().size(); ++j) {
                if (keyName == method.argNames().at(i)) {
                    Slot* arg = reinterpret_cast<Slot*>(reinterpret_cast<int8_t*>(calleeFrame.instance()) +
                        offsetof(schema::FramePrivateSchema, arg0));
                    *arg = keyValue;
                    break;
                }
            }
        }
    }

    auto scMethod = reinterpret_cast<SCMethod>(method.code().getRawPointer());
    return scMethod(context, calleeFrame.instance(), stackPointer);
}

bool ClassLibrary::resetLibrary(ThreadContext* context) {
    m_classMap.clear();
    m_classArray = library::ClassArray::typedArrayAlloc(context, 1);
    m_methodASTs.clear();
    m_methodFrames.clear();
    m_classVariables = library::Array();
    m_numberOfClassVariables = 0;
    return true;
}

bool ClassLibrary::scanClass(ThreadContext* context, library::Class classDef, library::Class metaClassDef,
                             const library::ClassNode classNode) {
    library::Symbol superclassName;
    library::Symbol metaSuperclassName;

    if (classNode.superclassNameToken()) {
        superclassName = classNode.superclassNameToken().snippet(context);
        metaSuperclassName = library::Symbol::fromView(context, fmt::format("Meta_{}", superclassName.view(context)));
    } else {
        if (classDef.name(context) == context->symbolTable->objectSymbol()) {
            // The superclass of 'Meta_Object' is 'Class'.
            metaSuperclassName = library::Symbol::fromView(context, "Class");
        } else {
            superclassName = library::Symbol::fromView(context, "Object");
            metaSuperclassName = library::Symbol::fromView(context, "Meta_Object");
        }
    }

    // Set up parent object and add this class definition to its subclasses array, if this isn't `Object`.
    if (classDef.name(context) != context->symbolTable->objectSymbol()) {
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
    library::VarListNode varList = classNode.variables();
    while (varList) {
        auto varType = varList.token().name(context);
        auto nameArray = library::SymbolArray();
        auto valueArray = library::Array();

        library::VarDefNode varDef = varList.definitions();
        while (varDef) {
            nameArray = nameArray.add(context, varDef.token().snippet(context));
            if (varDef.initialValue()) {
                ASTBuilder builder;
                Slot literal = Slot::makeNil();
                auto wasLiteral = builder.buildLiteral(context, varDef.initialValue(), literal);
                assert(wasLiteral);
                valueArray = valueArray.add(context, literal);
            } else {
                valueArray = valueArray.add(context, Slot::makeNil());
            }
            varDef = library::VarDefNode(varDef.next().slot());
        }

        assert(nameArray.size() == valueArray.size());

        // Each line gets its own varList parse node, so append to any existing arrays to preserve previous values.
        if (varType == context->symbolTable->varSymbol()) {
            if (!m_bootstrapClasses.count(classDef.name(context))) {
                classDef.setInstVarNames(classDef.instVarNames().addAll(context, nameArray));
            }
            classDef.setIprototype(classDef.iprototype().addAll(context, valueArray));
        } else if (varType == context->symbolTable->classvarSymbol()) {
            classDef.setClassVarNames(classDef.classVarNames().addAll(context, nameArray));
            classDef.setCprototype(classDef.cprototype().addAll(context, valueArray));
            m_numberOfClassVariables += nameArray.size();
        } else if (varType == context->symbolTable->constSymbol()) {
            classDef.setConstNames(classDef.constNames().addAll(context, nameArray));
            classDef.setConstValues(classDef.constValues().addAll(context, valueArray));
        } else {
            // Internal error with VarListNode pointing at a token that isn't 'var', 'classvar', or 'const'.
            assert(false);
        }

        varList = library::VarListNode(varList.next().slot());
    }

    return true;
}

library::Class ClassLibrary::findOrInitClass(ThreadContext* context, library::Symbol className) {
    auto iter = m_classMap.find(className);
    if (iter != m_classMap.end()) {
        return library::Class::wrapUnsafe(iter->second);
    }

    library::Class classDef = library::Class::alloc(context);
    classDef.initToNil();

    // We change the tags on the class objects to reflect the sclang requriements.
    if (className.isMetaClassName(context)) {
        classDef.instance()->schema.className = context->symbolTable->classSymbol().hash();
    } else {
        auto metaClassName = library::Symbol::fromView(context, fmt::format("Meta_{}", className.view(context)));
        classDef.instance()->schema.className = metaClassName.hash();
    }
    classDef.setName(className);

    m_classMap.emplace(std::make_pair(className, classDef.slot()));

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
    if (objectIter == m_classMap.end()) {
        assert(false);
        return false;
    }

    // Allocate class variable array at full capacity, to avoid any expensive resize copies while appending.
    m_classVariables = library::Array::arrayAlloc(context, m_numberOfClassVariables);

    // We start at the root of the class heirarchy with Object.
    auto objectClassDef = objectIter->second;
    bool success = composeSubclassesFrom(context, library::Class::wrapUnsafe(objectClassDef));

    // We've converted all the ASTs to Frames, so we can free up the RAM.
    m_methodASTs.clear();

    return success;
}

bool ClassLibrary::composeSubclassesFrom(ThreadContext* context, library::Class classDef) {
    auto classASTs = m_methodASTs.find(classDef.name(context));
    assert(classASTs != m_methodASTs.end());

    auto classMethods = m_methodFrames.find(classDef.name(context));
    assert(classMethods != m_methodFrames.end());

    // Add the class variables initial values to the class variable array.
    classDef.setClassVarIndex(m_classVariables.size());
    m_classVariables = m_classVariables.addAll(context, classDef.cprototype());

    for (int32_t i = 0; i < classDef.methods().size(); ++i) {
        auto method = classDef.methods().typedAt(i);

        // We don't compile methods that include primitives.
        if (method.primitiveName(context)) {
            continue;
        }

        auto methodName = method.name(context);

        auto astIter = classASTs->second->find(methodName);
        assert(astIter != classASTs->second->end());

        BlockBuilder blockBuilder(method);
        auto frame = blockBuilder.buildMethod(context, astIter->second, false);
        if (!frame) {
            return false;
        }

        // Copy some basic elements out of the frame into the Method data structure.
        method.setPrototypeFrame(frame.prototypeFrame());
        method.setArgNames(frame.argumentNames());
        method.setVarNames(frame.variableNames());

        // TODO: Here's where we could extract some message signatures and compute dependencies, to decide on final
        // ordering of compilation of methods to support inlining.

        classMethods->second->emplace(std::make_pair(method.name(context), frame));
    }

    for (int32_t i = 0; i < classDef.subclasses().size(); ++i) {
        auto subclass = classDef.subclasses().typedAt(i);

        if (!m_bootstrapClasses.count(subclass.name(context))) {
            subclass.setInstVarNames(classDef.instVarNames()
                                         .copy(context, classDef.instVarNames().size() + subclass.instVarNames().size())
                                         .addAll(context, subclass.instVarNames()));
        }

        subclass.setIprototype(classDef.iprototype()
                                   .copy(context, classDef.iprototype().size() + subclass.iprototype().size())
                                   .addAll(context, subclass.iprototype()));

        if (!composeSubclassesFrom(context, subclass)) {
            return false;
        }
    }

    return true;
}

bool ClassLibrary::materializeFrames(ThreadContext* context) {
    for (auto& methodMap : m_methodFrames) {
        auto className = methodMap.first;
        assert(m_classMap.find(className) != m_classMap.end());
        auto classDef = library::Class::wrapUnsafe(m_classMap.at(className));
        auto methods = classDef.methods();

        for (int32_t i = 0; i < methods.size(); ++i) {
            auto method = methods.typedAt(i);
            auto methodName = method.name(context);

            // Methods that call a primitive have no Frame and should not be compiled.
            auto frameIter = methodMap.second->find(methodName);
            if (frameIter == methodMap.second->end()) {
                continue;
            }

            auto jitMethod = context->generator->serialize(context, frameIter->second);
            assert(jitMethod);
            method.setCode(Slot::makeRawPointer(reinterpret_cast<int8_t*>(jitMethod)));
        }
    }

    return true;
}

bool ClassLibrary::cleanUp() { return true; }

} // namespace hadron