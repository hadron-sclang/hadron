#ifndef SRC_HADRON_LIBRARY_LIFETIME_INTERVAL_HPP_
#define SRC_HADRON_LIBRARY_LIFETIME_INTERVAL_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/Boolean.hpp"
#include "hadron/library/HadronLIR.hpp"
#include "hadron/library/Integer.hpp"
#include "hadron/library/Set.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/schema/HLang/HadronLifetimeIntervalSchema.hpp"

namespace hadron { namespace library {

class LiveRange : public Object<LiveRange, schema::HadronLiveRangeSchema> {
public:
    LiveRange(): Object<LiveRange, schema::HadronLiveRangeSchema>() { }
    explicit LiveRange(schema::HadronLiveRangeSchema* instance):
        Object<LiveRange, schema::HadronLiveRangeSchema>(instance) { }
    explicit LiveRange(Slot instance): Object<LiveRange, schema::HadronLiveRangeSchema>(instance) { }
    ~LiveRange() { }

    static LiveRange makeLiveRange(ThreadContext* context, int32_t from, int32_t to) {
        auto liveRange = LiveRange::alloc(context);
        liveRange.setFrom(Integer(from));
        liveRange.setTo(Integer(to));
        return liveRange;
    }

    Integer from() const { return Integer(m_instance->from); }
    void setFrom(Integer f) { m_instance->from = f.slot(); }

    Integer to() const { return Integer(m_instance->to); }
    void setTo(Integer t) { m_instance->to = t.slot(); }
};

class LifetimeInterval : public Object<LifetimeInterval, schema::HadronLifetimeIntervalSchema> {
public:
    LifetimeInterval(): Object<LifetimeInterval, schema::HadronLifetimeIntervalSchema>() { }
    explicit LifetimeInterval(schema::HadronLifetimeIntervalSchema* instance):
        Object<LifetimeInterval, schema::HadronLifetimeIntervalSchema>(instance) { }
    explicit LifetimeInterval(Slot instance):
        Object<LifetimeInterval, schema::HadronLifetimeIntervalSchema>(instance) { }
    ~LifetimeInterval() { }

    static LifetimeInterval makeLifetimeInterval(ThreadContext* context, VReg value = VReg()) {
        auto lifetimeInterval = LifetimeInterval::alloc(context);
        lifetimeInterval.initToNil();
        lifetimeInterval.setUsages(OrderedIdentitySet::makeIdentitySet(context));
        lifetimeInterval.setValueNumber(value);
        lifetimeInterval.setIsSplit(false);
        lifetimeInterval.setIsSpill(false);
        return lifetimeInterval;
    }

    // Adds an interval in sorted order to list, possibly merging with other intervals.
    void addLiveRange(ThreadContext* context, int32_t from, int32_t to);

    // Keeps all ranges before |splitTime|, return a new LifetimeInterval with all ranges after |splitTime|. If
    // |splitTime| is within a LiveRange it will also be split. Also splits the usages set.
    LifetimeInterval splitAt(ThreadContext* context, int32_t splitTime);

    // Returns true if p is within a LiveRange inside this LifetimeInterval.
    bool covers(int32_t p) const;

    // Returns true if this and |lt| intersect, meaning there is some value |first| contained in a LiveRange for
    // both objects. Will set |first| to that value if true, will not modify first if false.
    bool findFirstIntersection(const LifetimeInterval lt, int32_t& first) const;

    bool isEmpty() const { return ranges().size() == 0; }
    Integer start() const {
        if (isEmpty()) {
            return Integer();
        }
        return ranges().typedFirst().from();
    }
    Integer end() const {
        if (isEmpty()) {
            return Integer();
        }
        return ranges().typedLast().to();
    }

    TypedArray<LiveRange> ranges() const { return TypedArray<LiveRange>(m_instance->ranges); }
    void setRanges(TypedArray<LiveRange> r) { m_instance->ranges = r.slot(); }

    OrderedIdentitySet usages() const { return OrderedIdentitySet(m_instance->usages); }
    void setUsages(OrderedIdentitySet u) { m_instance->usages = u.slot(); }

    VReg valueNumber() const { return VReg(m_instance->valueNumber); }
    void setValueNumber(VReg v) { m_instance->valueNumber = v.slot(); }

    Integer registerNumber() const { return Integer(m_instance->registerNumber); }
    void setRegisterNumber(Integer r) { m_instance->registerNumber = r.slot(); }

    bool isSplit() const { return m_instance->isSplit.getBool(); }
    void setIsSplit(bool b) { m_instance->isSplit = Slot::makeBool(b); }

    bool isSpill() const { return m_instance->isSpill.getBool(); }
    void setIsSpill(bool b) { m_instance->isSpill = Slot::makeBool(b); }

    Integer spillSlot() const { return Integer(m_instance->spillSlot); }
    void setSpillSlot(Integer i) { m_instance->spillSlot = i.slot(); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_LIFETIME_INTERVAL_HPP_