#ifndef SRC_HADRON_LIBRARY_THREAD_HPP_
#define SRC_HADRON_LIBRARY_THREAD_HPP_

#include "hadron/library/Stream.hpp"
#include "hadron/schema/Common/Core/ThreadSchema.hpp"

namespace hadron {
namespace library {

template<typename T, typename S>
class ThreadBase : public Stream<T, S> {
public:
    ThreadBase(): Stream<T, S>() {}
    explicit ThreadBase(S* instance): Stream<T, S>(instance) {}
    explicit ThreadBase(Slot instance): Stream<T, S>(instance) {}
    ~ThreadBase() {}
};

class Thread : public ThreadBase<Thread, schema::ThreadSchema> {
public:
    Thread(): ThreadBase<Thread, schema::ThreadSchema>() {}
    explicit Thread(schema::ThreadSchema* instance): ThreadBase<Thread, schema::ThreadSchema>(instance) {}
    explicit Thread(Slot instance): ThreadBase<Thread, schema::ThreadSchema>(instance) {}
    ~Thread() {}
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_THREAD_HPP_