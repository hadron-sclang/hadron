#ifndef SRC_HADRON_ARCH_HPP_
#define SRC_HADRON_ARCH_HPP_

#include <cstddef>
#include <cstdint>

// Contains information about the CPU architecture hadron is running on.
namespace hadron {

static constexpr size_t kNumberOfReservedRegisters = 3;

#if defined(__i386__)
static constexpr size_t kNumberOfPhysicalRegisters = 8 - kNumberOfReservedRegisters;
static constexpr size_t kNumberOfPhysicalFloatRegisters = 8;
using Word = int32_t;
using UWord = uint32_t;
#elif defined(__x86_64__)
static constexpr size_t kNumberOfPhysicalRegisters = 16 - kNumberOfReservedRegisters;
static constexpr size_t kNumberOfPhysicalFloatRegisters = 16;
using Word = int64_t;
using UWord = uint64_t;
#elif defined(__arm__)
static constexpr size_t kNumberOfPhysicalRegisters = 16 - kNumberOfReservedRegisters;
static constexpr size_t kNumberOfPhysicalFloatRegisters = 32;
using Word = int32_t;
using UWord = uint32_t;
#elif defined(__aarch64__)
static constexpr size_t kNumberOfPhysicalRegisters = 32 - kNumberOfReservedRegisters;
static constexpr size_t kNumberOfPhysicalFloatRegisters = 32;
using Word = int64_t;
using UWord = uint64_t;
#else
#    error "Undefined chipset"
#endif

} // namespace hadron

#endif // SRC_HADRON_ARCH_HPP_