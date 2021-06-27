#ifndef SRC_TYPE_HPP_
#define SRC_TYPE_HPP_

#include <cstdint>

namespace hadron {

enum Type : std::uint_fast16_t {
    kNil     = 0x0001,
    kInteger = 0x0002,
    kFloat   = 0x0004,
    kBoolean = 0x0008,
    kString  = 0x0010,
    kSymbol  = 0x0020,
    kClass   = 0x0040,
    kObject  = 0x0080,
    kArray   = 0x0100,
    kSlot    = 0x01ff
};

} // namespace hadron

#endif // SRC_TYPE_HPP_
