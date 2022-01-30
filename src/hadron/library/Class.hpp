#ifndef SRC_HADRON_LIBRARY_CLASS_HPP_
#define SRC_HADRON_LIBRARY_CLASS_HPP_

#include "hadron/Hash.hpp"

#include "hadron/library/Object.hpp"

#include "schema/Common/Core/KernelSchema.hpp"

namespace hadron {
namespace library {

class Class : public Object<Class, schema::ClassSchema>{
public:
    Class() = delete;
    explicit Class(S* instance): Object<T,S>(instance) {}
    ~Class() {}

};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_CLASS_HPP_