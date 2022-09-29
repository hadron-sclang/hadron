#ifndef SRC_HADRON_LIBRARY_THREAD_HPP_
#define SRC_HADRON_LIBRARY_THREAD_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/Stream.hpp"
#include "hadron/schema/Common/Core/ThreadSchema.hpp"

namespace hadron { namespace library {

template <typename T, typename S> class ThreadBase : public Stream<T, S> {
public:
    ThreadBase(): Stream<T, S>() { }
    explicit ThreadBase(S* instance): Stream<T, S>(instance) { }
    explicit ThreadBase(Slot instance): Stream<T, S>(instance) { }
    ~ThreadBase() { }
};

class Thread : public ThreadBase<Thread, schema::ThreadSchema> {
public:
    Thread(): ThreadBase<Thread, schema::ThreadSchema>() { }
    explicit Thread(schema::ThreadSchema* instance): ThreadBase<Thread, schema::ThreadSchema>(instance) { }
    explicit Thread(Slot instance): ThreadBase<Thread, schema::ThreadSchema>(instance) { }
    ~Thread() { }

    static Thread makeThread(ThreadContext* context, int32_t stackSize = 512) {
        auto thread = Thread::alloc(context);
        thread.initToNil();
        thread.setState(ThreadState::kInit);
        thread.setStack(Array::newClear(context, stackSize));
        thread.setSp(thread.stack().start());
        return thread;
    }

    // TODO: the values enumerated in the sc documentation are as follows:
    // enum ThreadState : int32_t { kNotStarted = 0, kRunning = 7, kStopped = 8 };
    // But in reality in the sclang code (PyrKernel.h) they are an unspecified enum:
    // enum { tInit, tStart, tReady, tRunning, tSleeping, tSuspended, tDone }
    // And spot check of running thread code returns 3, so we model after existing code behavior
    // in terms of observability. Reconcile.
    enum ThreadState : int32_t { kInit = 0, kRunning = 3, kDone = 6 };
    ThreadState state() const { return static_cast<ThreadState>(m_instance->state.getInt32()); }
    void setState(ThreadState ts) { m_instance->state = Slot::makeInt32(static_cast<int32_t>(ts)); }

    Array stack() const { return Array(m_instance->stack); }
    void setStack(Array s) { m_instance->stack = s.slot(); }

    Slot* sp() const { return reinterpret_cast<Slot*>(m_instance->sp.getRawPointer()); }
    void setSp(Slot* p) { m_instance->sp = Slot::makeRawPointer(reinterpret_cast<int8_t*>(p)); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_THREAD_HPP_