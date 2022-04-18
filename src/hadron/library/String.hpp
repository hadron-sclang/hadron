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
    static String fromView(ThreadContext* context, std::string_view v, int32_t additionalSize = 0) {
        String s = String::arrayAlloc(context, v.size() + additionalSize);
        std::memcpy(s.start(), v.data(), v.size());
        s.m_instance->schema._sizeInBytes = sizeof(schema::StringSchema) + v.size();
        return s;
    }

    // Appends the data contained at |v| to this string. If |hasEscape| is true, will process the characters
    // individually and process any escape characters, otherwise it performs a batch copy.
    String appendView(ThreadContext* context, std::string_view v, bool hasEscape) {
        String string = *this;
        if (size() + static_cast<int32_t>(v.length()) > capacity(context)) {
            string = fromView(context, std::string_view(start(), size()), v.length());
        }

        if (!hasEscape) {
            string.m_instance->schema._sizeInBytes += v.length();
            std::memcpy(string.start() + string.size(), v.data(), v.length());
            return string;
        }

        const char* input = v.data();
        char* append = string.start() + string.size();
        while (input < v.data() + v.size()) {
            if (*input != '\\') {
                *append = *input;
            } else {
                ++input;
                switch (*input) {
                case 'r':
                    *append = '\r';
                    break;

                case 'n':
                    *append = '\n';
                    break;

                case 't':
                    *append = '\t';
                    break;

                default:
                    *append = *input;
                    break;
                }
            }

            ++append;
            ++input;
        }

        string.m_instance->schema._sizeInBytes += append - (string.start() + string.size());

        return string;
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