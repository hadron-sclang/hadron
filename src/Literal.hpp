#ifndef SRC_LITERAL_HPP_
#define SRC_LITERAL_HPP_

#include "Type.hpp"

#include <stdint.h>

namespace hadron {

// Represents a literal value in the input source code.
class Literal {
public:
    Literal(): m_type(kNil) {}
    Literal(int64_t value): m_type(kInteger), m_value(value) {}
    Literal(double value): m_type(kFloat), m_value(value) {}
    Literal(bool value): m_type(kBoolean), m_value(value) {}

    // Make an empty Literal with provided type.
    Literal(Type t): m_type(t) {}

    Type type() const { return m_type; }

    // The as* functions provide raw access to the underlying storage and do no validation. The to* functions do
    // conversions and so will return a valid result.
    int64_t asInteger() const { return m_value.integer; }
    double asFloat() const { return m_value.floatingPoint; }
    bool asBoolean() const { return m_value.boolean; }

private:
    Type m_type;
    union Value {
        Value(): integer(0) {}
        Value(int64_t v): integer(v) {}
        Value(double v): floatingPoint(v) {}
        Value(bool v): boolean(v) {}

        int64_t integer;
        double floatingPoint;
        bool boolean;
    };
    Value m_value;
};

} // namespace hadron

#endif // SRC_LITERAL_HPP_
