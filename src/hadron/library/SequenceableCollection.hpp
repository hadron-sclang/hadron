#ifndef SRC_HADRON_LIBRARY_SEQUENCEABLE_COLLECTION_HPP_
#define SRC_HADRON_LIBRARY_SEQUENCEABLE_COLLECTION_HPP_

#include "hadron/library/Collection.hpp"
#include "hadron/schema/Common/Collections/SequenceableCollectionSchema.hpp"

namespace hadron { namespace library {

template <typename T, typename S> class SequenceableCollection : public Collection<T, S> {
public:
    SequenceableCollection(): Collection<T, S>() { }
    explicit SequenceableCollection(S* instance): Collection<T, S>(instance) { }
    explicit SequenceableCollection(Slot instance): Collection<T, S>(instance) { }
    ~SequenceableCollection() { }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_SEQUENCEABLE_COLLECTION_HPP_