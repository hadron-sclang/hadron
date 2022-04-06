#ifndef SRC_HADRON_LIBRARY_STRING_HPP_
#define SRC_HADRON_LIBRARY_STRING_HPP_

#include "hadron/Slot.hpp"

#include "hadron/library/ArrayedCollection.hpp"
#include "hadron/schema/Common/Collections/StringSchema.hpp"

#include <cstring>
#include <string_view>

namespace hadron {
namespace library {

class String : public RawArray<String, schema::StringSchema, char> {
public:
    String(): RawArray<String, schema::StringSchema, char>() {}
    explicit String(schema::StringSchema* instance): RawArray<String, schema::StringSchema, char>(instance) {}
    explicit String(Slot instance): RawArray<String, schema::StringSchema, char>(instance) {}
    ~String() {}

    // Copies data into a heap-allocated string object.
    static String fromView(ThreadContext* context, std::string_view v) {
        String s = String::arrayAlloc(context, v.size());
        std::memcpy(s.start(), v.data(), v.size());
        s.m_instance->schema._sizeInBytes = sizeof(schema::StringSchema) + v.size();
        return s;
    }

    // Returns true if both strings are identical.
    bool compare(String s) const {
        if (size() != s.size()) return false;
        return std::memcmp(start(), s.start(), size()) == 0;
    }
    bool compare(std::string_view v) const {
        if (static_cast<size_t>(size()) != v.size()) return false;
        return std::memcmp(start(), v.data(), v.size()) == 0;
    }

    std::string_view view() const { return std::string_view(start(), size()); }

};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_STRING_HPP_