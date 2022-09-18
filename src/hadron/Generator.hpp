#ifndef SRC_HADRON_GENERATOR_HPP_
#define SRC_HADRON_GENERATOR_HPP_

// Architecture-specific Generator inclusion

#if defined(__i386__)
#elif defined(__x86_64__)
#elif defined(__arm__)
#elif defined(__aarch64__)
#include "hadron/aarch64/Generator.hpp"
#endif

#endif // SRC_HADRON_GENERATOR_HPP_
