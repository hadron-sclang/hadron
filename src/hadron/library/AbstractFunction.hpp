#ifndef SRC_HADRON_LIBRARY_ABSTRACT_FUNCTION_HPP_
#define SRC_HADRON_LIBRARY_ABSTRACT_FUNCTION_HPP_

#include "hadron/library/Object.hpp"
#include "hadron/schema/Common/Core/AbstractFunctionSchema.hpp"

namespace hadron { namespace library {

template <typename T, typename S> class AbstractFunction : public Object<T, S> {
public:
    AbstractFunction(): Object<T, S>() { }
    explicit AbstractFunction(S* instance): Object<T, S>(instance) { }
    explicit AbstractFunction(Slot instance): Object<T, S>(instance) { }
    ~AbstractFunction() { }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_ABSTRACT_FUNCTION_HPP_