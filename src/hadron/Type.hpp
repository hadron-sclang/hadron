#ifndef SRC_COMPILER_INCLUDE_HADRON_TYPE_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_TYPE_HPP_

#include <cstdint>

namespace hadron {

// These are deliberately independent bits to allow for quick aggregate type comparisons, such as
// type & (kInteger | kFloat) to determine if a type is numeric or
// type & (kString | kSymbol) for character types, etc.
enum Type : std::int32_t {
    kNil     = 0x0001,
    kInteger = 0x0002,
    kFloat   = 0x0004,
    kBoolean = 0x0008,
    kChar    = 0x0010,
    kString  = 0x0020,
    kSymbol  = 0x0040,
    kClass   = 0x0080,
    kObject  = 0x0100,
    kArray   = 0x0200,
    kBlock   = 0x0400,
    kAny     = 0x07ff,

    // Type is an internal-only flag used for type tracking in SSA, so we exclude it from the other flags.
    kType    = 0x1000
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_TYPE_HPP_
