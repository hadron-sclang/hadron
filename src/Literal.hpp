#ifndef SRC_LITERAL_HPP_
#define SRC_LITERAL_HPP_

#include "Type.hpp"

#include <stdint.h>

namespace hadron {

// Represents a literal value in the input source code.
class Literal {
public:
    Literal(): m_type(kNil) {}
    Literal(int32_t value): m_type(kInteger), m_value(value) {}
    Literal(float value): m_type(kFloat), m_value(value) {}
    Literal(bool value): m_type(kBoolean), m_value(value) {}

    // Make an empty Literal with provided type.
    Literal(Type t): m_type(t) {}
    // Make a string or symbol literal with a flag indicating if it needs escape processing.
    Literal(Type t, bool hasEscapeCharacters): m_type(t), m_value(hasEscapeCharacters) {}

    Type type() const { return m_type; }

    // The as* functions provide raw access to the underlying storage and do no validation. If needed, the to*
    // functions can do conversions and so will return a valid result.
    int32_t asInteger() const { return m_value.integer; }
    float asFloat() const { return m_value.floatingPoint; }
    bool asBoolean() const { return m_value.boolean; }

    // Assumes and does not validate that the underlying type is a String or Symbol
    bool hasEscapeCharacters() const { return m_value.boolean; }

private:
    Type m_type;
    union Value {
        Value(): integer(0) {}
        Value(int32_t v): integer(v) {}
        Value(float v): floatingPoint(v) {}
        Value(bool v): boolean(v) {}

        int32_t integer;
        float floatingPoint;
        bool boolean;
    };
    Value m_value;
};

} // namespace hadron

#endif // SRC_LITERAL_HPP_
