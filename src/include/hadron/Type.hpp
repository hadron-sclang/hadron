#ifndef SRC_TYPE_HPP_
#define SRC_TYPE_HPP_

#include <cstdint>

namespace hadron {

// These are deliberately independent bits to allow for quick aggregate type comparisons, such as
// type & (kInteger | kFloat) to determine if a type is numeric or
// type & (kString | kSymbol) for character types, etc.
enum Type : std::uint32_t {
    kNil     = 0x0001,
    kInteger = 0x0002,
    kFloat   = 0x0004,
    kBoolean = 0x0008,
    kString  = 0x0010,
    kSymbol  = 0x0020,
    kClass   = 0x0040,
    kObject  = 0x0080,
    kArray   = 0x0100,
    kSlot    = 0x01ff,

    // Internal implementation types
    kMachineCodePointer = 0x10000,
    kFramePointer       = 0x20000,
    kStackPointer       = 0x40000
};

} // namespace hadron

#endif // SRC_TYPE_HPP_
