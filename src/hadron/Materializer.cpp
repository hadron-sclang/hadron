#include "hadron/Materializer.hpp"

#include "hadron/Arch.hpp"
#include "hadron/BlockSerializer.hpp"
#include "hadron/Emitter.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/VirtualJIT.hpp"
#include "hadron/library/ArrayedCollection.hpp"
#include "hadron/library/HadronHIR.hpp"
#include "hadron/library/HadronLinearFrame.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
#include "hadron/RegisterAllocator.hpp"
#include "hadron/Resolver.hpp"
#include "hadron/ThreadContext.hpp"
#include <memory>

namespace hadron {

// static
library::Int8Array Materializer::materialize(ThreadContext* context, library::CFGFrame frame) {
    // Compile any inner blocks first.
    for (int32_t i = 0; i < frame.innerBlocks().size(); ++i) {
        auto innerBlock = frame.innerBlocks().typedAt(i);
        auto innerByteCode = Materializer::materialize(context, innerBlock.frame());
        auto functionDef = library::FunctionDef::alloc(context);
        functionDef.initToNil();
        functionDef.setCode(innerByteCode);
        functionDef.setSelectors(innerBlock.frame().selectors());
        functionDef.setPrototypeFrame(innerBlock.frame().prototypeFrame());

        // TODO: argNames, varNames?

        innerBlock.setFunctionDef(functionDef);
        frame.setSelectors(frame.selectors().typedAdd(context, functionDef));
    }

    BlockSerializer serializer;
    auto linearFrame = serializer.serialize(context, frame);

    LifetimeAnalyzer lifetimeAnalyzer;
    lifetimeAnalyzer.buildLifetimes(context, linearFrame);

    hadron::RegisterAllocator registerAllocator(hadron::kNumberOfPhysicalRegisters);
    registerAllocator.allocateRegisters(context, linearFrame);

    hadron::Resolver resolver;
    resolver.resolve(context, linearFrame);

    size_t bytecodeSize = linearFrame.instructions().size() * 16;
    library::Int8Array bytecode;
    std::unique_ptr<JIT> jit;

    if (context->debugMode) {
        bytecode = library::Int8Array::arrayAlloc(context, bytecodeSize);
        jit = std::make_unique<VirtualJIT>();
    } else {
        bytecode = library::Int8Array::arrayAllocJIT(context, bytecodeSize, bytecodeSize);
        jit = std::make_unique<LighteningJIT>();
    }

    jit->begin(bytecode.start(), bytecodeSize);
    Emitter emitter;
    emitter.emit(context, linearFrame, jit.get());

    assert(!jit->hasJITBufferOverflow());
    size_t finalSize = 0;
    jit->end(&finalSize);
    assert(finalSize < bytecodeSize);
    bytecode.resize(context, finalSize);

    return bytecode;
}

} // namespace hadron