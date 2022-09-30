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
    // TYPE CHECKING. Types are Float, Integer, Object, Symbol,

}

/*
template<typename... RET_ARGS>
class FuncSignatureT : public FuncSignature {
public:
  inline FuncSignatureT(CallConvId ccId = CallConvId::kHost, uint32_t vaIndex = kNoVarArgs) noexcept {
    static constexpr TypeId ret_args[] = { (TypeId(TypeUtils::TypeIdOfT<RET_ARGS>::kTypeId))... };
    init(ccId, vaIndex, ret_args[0], ret_args + 1, uint32_t(ASMJIT_ARRAY_SIZE(ret_args) - 1));
  }
};

//! Function signature builder.
class FuncSignatureBuilder : public FuncSignature {
public:
  TypeId _builderArgList[Globals::kMaxFuncArgs];

  //! \name Initializtion & Reset
  //! \{

  inline FuncSignatureBuilder(CallConvId ccId = CallConvId::kHost, uint32_t vaIndex = kNoVarArgs) noexcept {
    init(ccId, vaIndex, TypeId::kVoid, _builderArgList, 0);
  }

  //! \}

  //! \name Accessors
  //! \{

  //! Sets the return type to `retType`.
  inline void setRet(TypeId retType) noexcept { _ret = retType; }
  //! Sets the return type based on `T`.
  template<typename T>
  inline void setRetT() noexcept { setRet(TypeId(TypeUtils::TypeIdOfT<T>::kTypeId)); }

  //! Sets the argument at index `index` to `argType`.
  inline void setArg(uint32_t index, TypeId argType) noexcept {
    ASMJIT_ASSERT(index < _argCount);
    _builderArgList[index] = argType;
  }
  //! Sets the argument at index `i` to the type based on `T`.
  template<typename T>
  inline void setArgT(uint32_t index) noexcept { setArg(index, TypeId(TypeUtils::TypeIdOfT<T>::kTypeId)); }

  //! Appends an argument of `type` to the function prototype.
  inline void addArg(TypeId type) noexcept {
    ASMJIT_ASSERT(_argCount < Globals::kMaxFuncArgs);
    _builderArgList[_argCount++] = type;
  }
  //! Appends an argument of type based on `T` to the function prototype.
  template<typename T>
  inline void addArgT() noexcept { addArg(TypeId(TypeUtils::TypeIdOfT<T>::kTypeId)); }

  //! \}
};
*/



/*

#define ASMJIT_DEFINE_TYPE_ID(T, TYPE_ID) \
namespace TypeUtils {                     \
  template<>                              \
  struct TypeIdOfT<T> {                   \
    enum : uint32_t {                     \
      kTypeId = uint32_t(TYPE_ID)         \
    };                                    \
  };                                      \
}

ASMJIT_DEFINE_TYPE_ID(void         , TypeId::kVoid);
ASMJIT_DEFINE_TYPE_ID(Type::Bool   , TypeId::kUInt8);
ASMJIT_DEFINE_TYPE_ID(Type::Int8   , TypeId::kInt8);
ASMJIT_DEFINE_TYPE_ID(Type::UInt8  , TypeId::kUInt8);
ASMJIT_DEFINE_TYPE_ID(Type::Int16  , TypeId::kInt16);
ASMJIT_DEFINE_TYPE_ID(Type::UInt16 , TypeId::kUInt16);
ASMJIT_DEFINE_TYPE_ID(Type::Int32  , TypeId::kInt32);
ASMJIT_DEFINE_TYPE_ID(Type::UInt32 , TypeId::kUInt32);
ASMJIT_DEFINE_TYPE_ID(Type::Int64  , TypeId::kInt64);
ASMJIT_DEFINE_TYPE_ID(Type::UInt64 , TypeId::kUInt64);
ASMJIT_DEFINE_TYPE_ID(Type::IntPtr , TypeId::kIntPtr);
ASMJIT_DEFINE_TYPE_ID(Type::UIntPtr, TypeId::kUIntPtr);
ASMJIT_DEFINE_TYPE_ID(Type::Float32, TypeId::kFloat32);
ASMJIT_DEFINE_TYPE_ID(Type::Float64, TypeId::kFloat64);

*/

// Return type is always Slot. _this is always Slot. int32_t is the thing we need, and we need a pointer to _BasicNew
// that is also associated with the symbol name.

// Slot _BasicNew(Slot _this, int32_t maxSize)

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_PRIMITIVE_HPP_