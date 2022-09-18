#ifndef SRC_HADRON_LIBRARY_STREAM_HPP_
#define SRC_HADRON_LIBRARY_STREAM_HPP_

#include "hadron/library/AbstractFunction.hpp"
#include "hadron/schema/Common/Streams/StreamSchema.hpp"

namespace hadron { namespace library {

template <typename T, typename S> class Stream : public AbstractFunction<T, S> {
public:
    Stream(): AbstractFunction<T, S>() { }
    explicit Stream(S* instance): AbstractFunction<T, S>(instance) { }
    explicit Stream(Slot instance): AbstractFunction<T, S>(instance) { }
    ~Stream() { }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_STREAM_HPP_