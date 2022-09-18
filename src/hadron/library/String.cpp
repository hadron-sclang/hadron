#include "hadron/library/String.hpp"

#include "hadron/library/Symbol.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"

#include <cstring>

namespace hadron { namespace library {

// static
String String::fromView(ThreadContext* context, std::string_view v, int32_t additionalSize) {
    String s = String::arrayAlloc(context, v.size() + additionalSize);
    std::memcpy(s.start(), v.data(), v.size());
    s.m_instance->schema._sizeInBytes = sizeof(schema::StringSchema) + v.size();
    return s;
}

String String::appendView(ThreadContext* context, std::string_view v, bool hasEscape) {
    String string = *this;
    if (size() + static_cast<int32_t>(v.length()) > capacity(context)) {
        string = fromView(context, std::string_view(start(), size()), v.length());
    }

    if (!hasEscape) {
        std::memcpy(string.start() + string.size(), v.data(), v.length());
        string.m_instance->schema._sizeInBytes += v.length();
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

} // namespace library
} // namespace hadron