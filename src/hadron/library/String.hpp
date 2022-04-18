#ifndef SRC_HADRON_LIBRARY_STRING_HPP_
#define SRC_HADRON_LIBRARY_STRING_HPP_

#include "hadron/Slot.hpp"

#include "hadron/library/ArrayedCollection.hpp"
#include "hadron/schema/Common/Collections/StringSchema.hpp"

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
    static String fromView(ThreadContext* context, std::string_view v, int32_t additionalSize = 0);

    // Appends the data contained at |v| to this string. If |hasEscape| is true, will process the characters
    // individually and process any escape characters, otherwise it performs a batch copy.
    String appendView(ThreadContext* context, std::string_view v, bool hasEscape);

    // Returns true if both strings are identical.
    inline bool compare(String s) const {
        if (size() != s.size()) return false;
        return std::memcmp(start(), s.start(), size()) == 0;
    }
    inline bool compare(std::string_view v) const {
        if (static_cast<size_t>(size()) != v.size()) return false;
        return std::memcmp(start(), v.data(), v.size()) == 0;
    }

    std::string_view view() const { return std::string_view(start(), size()); }

};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_STRING_HPP_