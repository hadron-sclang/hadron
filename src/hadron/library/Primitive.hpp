#ifndef SRC_HADRON_LIBRARY_PRIMITIVE_HPP_
#define SRC_HADRON_LIBRARY_PRIMITIVE_HPP_

namespace hadron { namespace library {

// schemac time
// build code to bind symbol '_BasicNew' to a function pointer
// maybe Generator.hpp also provides a templatized function to JIT bindings and invoke
template<typename T ... Ts>
SCMethod* bindPrimitive(void* functionPointer) {
    asmjit::CodeHolder codeHolder;
    codeHolder.init(m_jitRuntime.environment());

    asmjit::x86::Compiler compiler(&codeHolder);
    auto funcNode = compiler.addFunc(signature);  // this is the standard signature..

    asmjit::x86::Gp contextReg = compiler.newGp(asmjit::TypeId::kIntPtr);
    funcNode->setArg(0, contextReg);
    asmjit::x86::Gp framePointerReg = compiler.newGp(asmjit::TypeId::kIntPtr);
    funcNode->setArg(1, framePointerReg);
    asmjit::x86::Gp stackPointerReg = compiler.newGp(asmjit::TypeId::kIntPtr);
    funcNode->setArg(2, stackPointerReg);

    // type-specific packing
    vector<asmjit::x86::Gp> gps;
    
    gps.push_back(compiler.newGp(kUint64));
    compiler.mov(gps[0], asmjit::x86::ptr(stackPointer, 0));
    // TYPE CHECKING

}

// Return type is always Slot. _this is always Slot. int32_t is the thing we need, and we need a pointer to _BasicNew
// that is also associated with the symbol name.

// Slot _BasicNew(Slot _this, int32_t maxSize)

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_PRIMITIVE_HPP_