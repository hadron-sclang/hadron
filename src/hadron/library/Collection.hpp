#ifndef SRC_HADRON_LIBRARY_COLLECTION_HPP_
#define SRC_HADRON_LIBRARY_COLLECTION_HPP_

#include "hadron/library/Object.hpp"
#include "schema/Common/Collections/CollectionSchema.hpp"

namespace hadron {
namespace library {

template<typename T, typename S>
class Collection : public Object<T, S> {
public:
    Collection() = delete;
    explicit Collection(S* instance): Object<T, S>(instance) {}
    ~Collection() {}
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_COLLECTION_HPP_