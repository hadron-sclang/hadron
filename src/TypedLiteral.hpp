#ifndef SRC_TYPED_LITERAL_HPP_
#define SRC_TYPED_LITERAL_HPP_

#include <stdint.h>

namespace hadron {

// Used in lexing and parsing, includes type information 
class TypedLiteral {
public:
    enum Type {
        kNil,
        kInteger,
        kFloat,
        kBoolean,

        // TODO: implement actual storage for these values with some kind of elementary garbage collection
        kString,
        kSymbol,

        kClass,
        kObject

        // maybe kArray?
    };

    TypedLiteral(): m_type(kNil) {}
    TypedLiteral(int64_t value): m_type(kInteger), m_value(value) {}
    TypedLiteral(double value): m_type(kFloat), m_value(value) {}
    TypedLiteral(bool value): m_type(kBoolean), m_value(value) {}

    // Make an empty TypedLiteral with provided type.
    TypedLiteral(Type t): m_type(t) {}

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

#endif // SRC_TYPED_LITERAL_HPP_
