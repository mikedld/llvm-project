//===- LLVMDialect.h - MLIR LLVM dialect types ------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the types for the LLVM dialect in MLIR. These MLIR types
// correspond to the LLVM IR type system.
//
//===----------------------------------------------------------------------===//

#ifndef MLIR_DIALECT_LLVMIR_LLVMTYPES_H_
#define MLIR_DIALECT_LLVMIR_LLVMTYPES_H_

#include "mlir/IR/Types.h"

namespace llvm {
class ElementCount;
} // namespace llvm

namespace mlir {

class DialectAsmParser;
class DialectAsmPrinter;

namespace LLVM {
class LLVMDialect;

namespace detail {
struct LLVMFunctionTypeStorage;
struct LLVMIntegerTypeStorage;
struct LLVMPointerTypeStorage;
struct LLVMStructTypeStorage;
struct LLVMTypeAndSizeStorage;
} // namespace detail

class LLVMBFloatType;
class LLVMHalfType;
class LLVMFloatType;
class LLVMDoubleType;
class LLVMIntegerType;

//===----------------------------------------------------------------------===//
// LLVMType.
//===----------------------------------------------------------------------===//

/// Base class for LLVM dialect types.
///
/// The LLVM dialect in MLIR fully reflects the LLVM IR type system, prodiving a
/// sperate MLIR type for each LLVM IR type. All types are represted as separate
/// subclasses and are compatible with the isa/cast infrastructure. For
/// convenience, the base class provides most of the APIs available on
/// llvm::Type in addition to MLIR-compatible APIs.
///
/// The LLVM dialect type system is closed: parametric types can only refer to
/// other LLVM dialect types. This is consistent with LLVM IR and enables a more
/// concise pretty-printing format.
///
/// Similarly to other MLIR types, LLVM dialect types are owned by the MLIR
/// context, have an immutable identifier (for most types except identified
/// structs, the entire type is the identifier) and are thread-safe.
class LLVMType : public Type::TypeBase<LLVMType, Type, TypeStorage> {
public:
  enum Kind {
    // Keep non-parametric types contiguous in the enum.
    VoidType = FIRST_LLVM_TYPE + 1,
    HalfType,
    BFloatType,
    FloatType,
    DoubleType,
    FP128Type,
    X86FP80Type,
    PPCFP128Type,
    X86MMXType,
    LabelType,
    TokenType,
    MetadataType,
    // End of non-parametric types.
    FunctionType,
    IntegerType,
    PointerType,
    FixedVectorType,
    ScalableVectorType,
    ArrayType,
    StructType,
    FIRST_NEW_LLVM_TYPE = VoidType,
    LAST_NEW_LLVM_TYPE = StructType,
    FIRST_TRIVIAL_TYPE = VoidType,
    LAST_TRIVIAL_TYPE = MetadataType
  };

  /// Inherit base constructors.
  using Base::Base;

  /// Support for PointerLikeTypeTraits.
  using Type::getAsOpaquePointer;
  static LLVMType getFromOpaquePointer(const void *ptr) {
    return LLVMType(static_cast<ImplType *>(const_cast<void *>(ptr)));
  }

  /// Support for isa/cast.
  static bool kindof(unsigned kind) {
    return FIRST_NEW_LLVM_TYPE <= kind && kind <= LAST_NEW_LLVM_TYPE;
  }

  LLVMDialect &getDialect();

  /// Floating-point type utilities.
  bool isBFloatTy() { return isa<LLVMBFloatType>(); }
  bool isHalfTy() { return isa<LLVMHalfType>(); }
  bool isFloatTy() { return isa<LLVMFloatType>(); }
  bool isDoubleTy() { return isa<LLVMDoubleType>(); }
  bool isFloatingPointTy() {
    return isa<LLVMHalfType>() || isa<LLVMBFloatType>() ||
           isa<LLVMFloatType>() || isa<LLVMDoubleType>();
  }

  /// Array type utilities.
  LLVMType getArrayElementType();
  unsigned getArrayNumElements();
  bool isArrayTy();

  /// Integer type utilities.
  bool isIntegerTy() { return isa<LLVMIntegerType>(); }
  bool isIntegerTy(unsigned bitwidth);
  unsigned getIntegerBitWidth();

  /// Vector type utilities.
  LLVMType getVectorElementType();
  unsigned getVectorNumElements();
  llvm::ElementCount getVectorElementCount();
  bool isVectorTy();

  /// Function type utilities.
  LLVMType getFunctionParamType(unsigned argIdx);
  unsigned getFunctionNumParams();
  LLVMType getFunctionResultType();
  bool isFunctionTy();
  bool isFunctionVarArg();

  /// Pointer type utilities.
  LLVMType getPointerTo(unsigned addrSpace = 0);
  LLVMType getPointerElementTy();
  bool isPointerTy();
  static bool isValidPointerElementType(LLVMType type);

  /// Struct type utilities.
  LLVMType getStructElementType(unsigned i);
  unsigned getStructNumElements();
  bool isStructTy();

  /// Utilities used to generate floating point types.
  static LLVMType getDoubleTy(LLVMDialect *dialect);
  static LLVMType getFloatTy(LLVMDialect *dialect);
  static LLVMType getBFloatTy(LLVMDialect *dialect);
  static LLVMType getHalfTy(LLVMDialect *dialect);
  static LLVMType getFP128Ty(LLVMDialect *dialect);
  static LLVMType getX86_FP80Ty(LLVMDialect *dialect);

  /// Utilities used to generate integer types.
  static LLVMType getIntNTy(LLVMDialect *dialect, unsigned numBits);
  static LLVMType getInt1Ty(LLVMDialect *dialect) {
    return getIntNTy(dialect, /*numBits=*/1);
  }
  static LLVMType getInt8Ty(LLVMDialect *dialect) {
    return getIntNTy(dialect, /*numBits=*/8);
  }
  static LLVMType getInt8PtrTy(LLVMDialect *dialect) {
    return getInt8Ty(dialect).getPointerTo();
  }
  static LLVMType getInt16Ty(LLVMDialect *dialect) {
    return getIntNTy(dialect, /*numBits=*/16);
  }
  static LLVMType getInt32Ty(LLVMDialect *dialect) {
    return getIntNTy(dialect, /*numBits=*/32);
  }
  static LLVMType getInt64Ty(LLVMDialect *dialect) {
    return getIntNTy(dialect, /*numBits=*/64);
  }

  /// Utilities used to generate other miscellaneous types.
  static LLVMType getArrayTy(LLVMType elementType, uint64_t numElements);
  static LLVMType getFunctionTy(LLVMType result, ArrayRef<LLVMType> params,
                                bool isVarArg);
  static LLVMType getFunctionTy(LLVMType result, bool isVarArg) {
    return getFunctionTy(result, llvm::None, isVarArg);
  }
  static LLVMType getStructTy(LLVMDialect *dialect, ArrayRef<LLVMType> elements,
                              bool isPacked = false);
  static LLVMType getStructTy(LLVMDialect *dialect, bool isPacked = false) {
    return getStructTy(dialect, llvm::None, isPacked);
  }
  template <typename... Args>
  static typename std::enable_if<llvm::are_base_of<LLVMType, Args...>::value,
                                 LLVMType>::type
  getStructTy(LLVMType elt1, Args... elts) {
    SmallVector<LLVMType, 8> fields({elt1, elts...});
    return getStructTy(&elt1.getDialect(), fields);
  }
  static LLVMType getVectorTy(LLVMType elementType, unsigned numElements);

  /// Void type utilities.
  static LLVMType getVoidTy(LLVMDialect *dialect);
  bool isVoidTy();

  // Creation and setting of LLVM's identified struct types
  static LLVMType createStructTy(LLVMDialect *dialect,
                                 ArrayRef<LLVMType> elements,
                                 Optional<StringRef> name,
                                 bool isPacked = false);

  static LLVMType createStructTy(LLVMDialect *dialect,
                                 Optional<StringRef> name) {
    return createStructTy(dialect, llvm::None, name);
  }

  static LLVMType createStructTy(ArrayRef<LLVMType> elements,
                                 Optional<StringRef> name,
                                 bool isPacked = false) {
    assert(!elements.empty() &&
           "This method may not be invoked with an empty list");
    LLVMType ele0 = elements.front();
    return createStructTy(&ele0.getDialect(), elements, name, isPacked);
  }

  template <typename... Args>
  static typename std::enable_if_t<llvm::are_base_of<LLVMType, Args...>::value,
                                   LLVMType>
  createStructTy(StringRef name, LLVMType elt1, Args... elts) {
    SmallVector<LLVMType, 8> fields({elt1, elts...});
    Optional<StringRef> opt_name(name);
    return createStructTy(&elt1.getDialect(), fields, opt_name);
  }

  static LLVMType setStructTyBody(LLVMType structType,
                                  ArrayRef<LLVMType> elements,
                                  bool isPacked = false);

  template <typename... Args>
  static typename std::enable_if_t<llvm::are_base_of<LLVMType, Args...>::value,
                                   LLVMType>
  setStructTyBody(LLVMType structType, LLVMType elt1, Args... elts) {
    SmallVector<LLVMType, 8> fields({elt1, elts...});
    return setStructTyBody(structType, fields);
  }
};

//===----------------------------------------------------------------------===//
// Trivial types.
//===----------------------------------------------------------------------===//

// Batch-define trivial types.
#define DEFINE_TRIVIAL_LLVM_TYPE(ClassName, Kind)                              \
  class ClassName : public Type::TypeBase<ClassName, LLVMType, TypeStorage> {  \
  public:                                                                      \
    using Base::Base;                                                          \
    static bool kindof(unsigned kind) { return kind == Kind; }                 \
    static ClassName get(MLIRContext *context) {                               \
      return Base::get(context, Kind);                                         \
    }                                                                          \
  }

DEFINE_TRIVIAL_LLVM_TYPE(LLVMVoidType, LLVMType::VoidType);
DEFINE_TRIVIAL_LLVM_TYPE(LLVMHalfType, LLVMType::HalfType);
DEFINE_TRIVIAL_LLVM_TYPE(LLVMBFloatType, LLVMType::BFloatType);
DEFINE_TRIVIAL_LLVM_TYPE(LLVMFloatType, LLVMType::FloatType);
DEFINE_TRIVIAL_LLVM_TYPE(LLVMDoubleType, LLVMType::DoubleType);
DEFINE_TRIVIAL_LLVM_TYPE(LLVMFP128Type, LLVMType::FP128Type);
DEFINE_TRIVIAL_LLVM_TYPE(LLVMX86FP80Type, LLVMType::X86FP80Type);
DEFINE_TRIVIAL_LLVM_TYPE(LLVMPPCFP128Type, LLVMType::PPCFP128Type);
DEFINE_TRIVIAL_LLVM_TYPE(LLVMX86MMXType, LLVMType::X86MMXType);
DEFINE_TRIVIAL_LLVM_TYPE(LLVMTokenType, LLVMType::TokenType);
DEFINE_TRIVIAL_LLVM_TYPE(LLVMLabelType, LLVMType::LabelType);
DEFINE_TRIVIAL_LLVM_TYPE(LLVMMetadataType, LLVMType::MetadataType);

#undef DEFINE_TRIVIAL_LLVM_TYPE

//===----------------------------------------------------------------------===//
// LLVMArrayType.
//===----------------------------------------------------------------------===//

/// LLVM dialect array type. It is an aggregate type representing consecutive
/// elements in memory, parameterized by the number of elements and the element
/// type.
class LLVMArrayType : public Type::TypeBase<LLVMArrayType, LLVMType,
                                            detail::LLVMTypeAndSizeStorage> {
public:
  /// Inherit base constructors.
  using Base::Base;

  /// Support for isa/cast.
  static bool kindof(unsigned kind) { return kind == LLVMType::ArrayType; }

  /// Gets or creates an instance of LLVM dialect array type containing
  /// `numElements` of `elementType`, in the same context as `elementType`.
  static LLVMArrayType get(LLVMType elementType, unsigned numElements);

  /// Returns the element type of the array.
  LLVMType getElementType();

  /// Returns the number of elements in the array type.
  unsigned getNumElements();
};

//===----------------------------------------------------------------------===//
// LLVMFunctionType.
//===----------------------------------------------------------------------===//

/// LLVM dialect function type. It consists of a single return type (unlike MLIR
/// which can have multiple), a list of parameter types and can optionally be
/// variadic.
class LLVMFunctionType
    : public Type::TypeBase<LLVMFunctionType, LLVMType,
                            detail::LLVMFunctionTypeStorage> {
public:
  /// Inherit base constructors.
  using Base::Base;

  /// Support for isa/cast.
  static bool kindof(unsigned kind) { return kind == LLVMType::FunctionType; }

  /// Gets or creates an instance of LLVM dialect function in the same context
  /// as the `result` type.
  static LLVMFunctionType get(LLVMType result, ArrayRef<LLVMType> arguments,
                              bool isVarArg = false);

  /// Returns the result type of the function.
  LLVMType getReturnType();

  /// Returns the number of arguments to the function.
  unsigned getNumParams();

  /// Returns `i`-th argument of the function. Asserts on out-of-bounds.
  LLVMType getParamType(unsigned i);

  /// Returns whether the function is variadic.
  bool isVarArg();

  /// Returns a list of argument types of the function.
  ArrayRef<LLVMType> getParams();
  ArrayRef<LLVMType> params() { return getParams(); }
};

//===----------------------------------------------------------------------===//
// LLVMIntegerType.
//===----------------------------------------------------------------------===//

/// LLVM dialect signless integer type parameterized by bitwidth.
class LLVMIntegerType : public Type::TypeBase<LLVMIntegerType, LLVMType,
                                              detail::LLVMIntegerTypeStorage> {
public:
  /// Inherit base constructor.
  using Base::Base;

  /// Support for isa/cast.
  static bool kindof(unsigned kind) { return kind == LLVMType::IntegerType; }

  /// Gets or creates an instance of the integer of the specified `bitwidth` in
  /// the given context.
  static LLVMIntegerType get(MLIRContext *ctx, unsigned bitwidth);

  /// Returns the bitwidth of this integer type.
  unsigned getBitWidth();
};

//===----------------------------------------------------------------------===//
// LLVMPointerType.
//===----------------------------------------------------------------------===//

/// LLVM dialect pointer type. This type typically represents a reference to an
/// object in memory. It is parameterized by the element type and the address
/// space.
class LLVMPointerType : public Type::TypeBase<LLVMPointerType, LLVMType,
                                              detail::LLVMPointerTypeStorage> {
public:
  /// Inherit base constructors.
  using Base::Base;

  /// Support for isa/cast.
  static bool kindof(unsigned kind) { return kind == LLVMType::PointerType; }

  /// Gets or creates an instance of LLVM dialect pointer type pointing to an
  /// object of `pointee` type in the given address space. The pointer type is
  /// created in the same context as `pointee`.
  static LLVMPointerType get(LLVMType pointee, unsigned addressSpace = 0);

  /// Returns the pointed-to type.
  LLVMType getElementType();

  /// Returns the address space of the pointer.
  unsigned getAddressSpace();
};

//===----------------------------------------------------------------------===//
// LLVMStructType.
//===----------------------------------------------------------------------===//

/// LLVM dialect structure type representing a collection of different-typed
/// elements manipulated together. Structured can optionally be packed, meaning
/// that their elements immediately follow each other in memory without
/// accounting for potential alignment.
///
/// Structure types can be identified (named) or literal. Literal structures
/// are uniquely represented by the list of types they contain and packedness.
/// Literal structure types are immutable after construction.
///
/// Identified structures are uniquely represented by their name, a string. They
/// have a mutable component, consisting of the list of types they contain,
/// the packedness and the opacity bits. Identified structs can be created
/// without providing the lists of element types, making them suitable to
/// represent recursive, i.e. self-referring, structures. Identified structs
/// without body are considered opaque. For such structs, one can set the body.
/// Identified structs can be created as intentionally-opaque, implying that the
/// caller does not intend to ever set the body (e.g. forward-declarations of
/// structs from another module) and wants to disallow further modification of
/// the body. For intentionally-opaque structs or non-opaque structs with the
/// body, one is not allowed to set another body (however, one can set exactly
/// the same body).
///
/// Note that the packedness of the struct takes place in uniquing of literal
/// structs, but does not in uniquing of identified structs.
class LLVMStructType : public Type::TypeBase<LLVMStructType, LLVMType,
                                             detail::LLVMStructTypeStorage> {
public:
  /// Inherit base construtors.
  using Base::Base;

  /// Support for isa/cast.
  static bool kindof(unsigned kind) { return kind == LLVMType::StructType; }

  /// Gets or creates an identified struct with the given name in the provided
  /// context. Note that unlike llvm::StructType::create, this function will
  /// _NOT_ rename a struct in case a struct with the same name already exists
  /// in the context. Instead, it will just return the existing struct,
  /// similarly to the rest of MLIR type ::get methods.
  static LLVMStructType getIdentified(MLIRContext *context, StringRef name);

  /// Gets or creates a literal struct with the given body in the provided
  /// context.
  static LLVMStructType getLiteral(MLIRContext *context,
                                   ArrayRef<LLVMType> types,
                                   bool isPacked = false);

  /// Gets or creates an intentionally-opaque identified struct. Such a struct
  /// cannot have its body set. To create an opaque struct with a mutable body,
  /// use `getIdentified`. Note that unlike llvm::StructType::create, this
  /// function will _NOT_ rename a struct in case a struct with the same name
  /// already exists in the context. Instead, it will just return the existing
  /// struct, similarly to the rest of MLIR type ::get methods.
  static LLVMStructType getOpaque(StringRef name, MLIRContext *context);

  /// Set the body of an identified struct. Returns failure if the body could
  /// not be set, e.g. if the struct already has a body or if it was marked as
  /// intentionally opaque. This might happen in a multi-threaded context when a
  /// different thread modified the struct after it was created. Most callers
  /// are likely to assert this always succeeds, but it is possible to implement
  /// a local renaming scheme based on the result of this call.
  LogicalResult setBody(ArrayRef<LLVMType> types, bool isPacked);

  /// Checks if a struct is packed.
  bool isPacked();

  /// Checks if a struct is identified.
  bool isIdentified();

  /// Checks if a struct is opaque.
  bool isOpaque();

  /// Checks if a struct is initialized.
  bool isInitialized();

  /// Returns the name of an identified struct.
  StringRef getName();

  /// Returns the list of element types contained in a non-opaque struct.
  ArrayRef<LLVMType> getBody();
};

//===----------------------------------------------------------------------===//
// LLVMVectorType.
//===----------------------------------------------------------------------===//

/// LLVM dialect vector type, represents a sequence of elements that can be
/// processed as one, typically in SIMD context. This is a base class for fixed
/// and scalable vectors.
class LLVMVectorType : public Type::TypeBase<LLVMVectorType, LLVMType,
                                             detail::LLVMTypeAndSizeStorage> {
public:
  /// Inherit base constructor.
  using Base::Base;

  /// Support for isa/cast.
  static bool kindof(unsigned kind) {
    return kind == LLVMType::FixedVectorType ||
           kind == LLVMType::ScalableVectorType;
  }

  /// Returns the element type of the vector.
  LLVMType getElementType();

  /// Returns the number of elements in the vector.
  llvm::ElementCount getElementCount();
};

//===----------------------------------------------------------------------===//
// LLVMFixedVectorType.
//===----------------------------------------------------------------------===//

/// LLVM dialect fixed vector type, represents a sequence of elements of known
/// length that can be processed as one.
class LLVMFixedVectorType
    : public Type::TypeBase<LLVMFixedVectorType, LLVMVectorType,
                            detail::LLVMTypeAndSizeStorage> {
public:
  /// Inherit base constructor.
  using Base::Base;

  /// Support for isa/cast.
  static bool kindof(unsigned kind) {
    return kind == LLVMType::FixedVectorType;
  }

  /// Gets or creates a fixed vector type containing `numElements` of
  /// `elementType` in the same context as `elementType`.
  static LLVMFixedVectorType get(LLVMType elementType, unsigned numElements);

  /// Returns the number of elements in the fixed vector.
  unsigned getNumElements();
};

//===----------------------------------------------------------------------===//
// LLVMScalableVectorType.
//===----------------------------------------------------------------------===//

/// LLVM dialect scalable vector type, represents a sequence of elements of
/// unknown length that is known to be divisible by some constant. These
/// elements can be processed as one in SIMD context.
class LLVMScalableVectorType
    : public Type::TypeBase<LLVMScalableVectorType, LLVMVectorType,
                            detail::LLVMTypeAndSizeStorage> {
public:
  /// Inherit base constructor.
  using Base::Base;

  /// Support for isa/cast.
  static bool kindof(unsigned kind) {
    return kind == LLVMType::ScalableVectorType;
  }

  /// Gets or creates a scalable vector type containing a non-zero multiple of
  /// `minNumElements` of `elementType` in the same context as `elementType`.
  static LLVMScalableVectorType get(LLVMType elementType,
                                    unsigned minNumElements);

  /// Returns the scaling factor of the number of elements in the vector. The
  /// vector contains at least the resulting number of elements, or any non-zero
  /// multiple of this number.
  unsigned getMinNumElements();
};

//===----------------------------------------------------------------------===//
// Printing and parsing.
//===----------------------------------------------------------------------===//

namespace detail {
/// Parses an LLVM dialect type.
LLVMType parseType(DialectAsmParser &parser);

/// Prints an LLVM Dialect type.
void printType(LLVMType type, DialectAsmPrinter &printer);
} // namespace detail

} // namespace LLVM
} // namespace mlir

//===----------------------------------------------------------------------===//
// Support for hashing and containers.
//===----------------------------------------------------------------------===//

namespace llvm {

// LLVMType instances hash just like pointers.
template <>
struct DenseMapInfo<mlir::LLVM::LLVMType> {
  static mlir::LLVM::LLVMType getEmptyKey() {
    void *pointer = llvm::DenseMapInfo<void *>::getEmptyKey();
    return mlir::LLVM::LLVMType(
        static_cast<mlir::LLVM::LLVMType::ImplType *>(pointer));
  }
  static mlir::LLVM::LLVMType getTombstoneKey() {
    void *pointer = llvm::DenseMapInfo<void *>::getTombstoneKey();
    return mlir::LLVM::LLVMType(
        static_cast<mlir::LLVM::LLVMType::ImplType *>(pointer));
  }
  static unsigned getHashValue(mlir::LLVM::LLVMType val) {
    return mlir::hash_value(val);
  }
  static bool isEqual(mlir::LLVM::LLVMType lhs, mlir::LLVM::LLVMType rhs) {
    return lhs == rhs;
  }
};

// LLVMType behaves like a pointer similarly to mlir::Type.
template <>
struct PointerLikeTypeTraits<mlir::LLVM::LLVMType> {
  static inline void *getAsVoidPointer(mlir::LLVM::LLVMType type) {
    return const_cast<void *>(type.getAsOpaquePointer());
  }
  static inline mlir::LLVM::LLVMType getFromVoidPointer(void *ptr) {
    return mlir::LLVM::LLVMType::getFromOpaquePointer(ptr);
  }
  static constexpr int NumLowBitsAvailable =
      PointerLikeTypeTraits<mlir::Type>::NumLowBitsAvailable;
};

} // namespace llvm

#endif // MLIR_DIALECT_LLVMIR_LLVMTYPES_H_
