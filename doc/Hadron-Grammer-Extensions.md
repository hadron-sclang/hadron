# Hadron Grammer Extensions

## Type Formatting

Hadron is *strongly typed*, meaning that the type of every expression in the language must be known at compile time.
This is different than the SuperCollider language, which has *dynamic typing.* There are many advantages to static
typing in programming languages, but they do require the programmer to provide additional information in the code about
the type of each expression.

Not all types have to be specified, some can be inferred by the compiler. Because the type syntax extends existing
SuperCollider grammar, and we can infer some types, we hope that SuperCollider code can be used in Hadron with only
minimal effort to add the missing type information.

### Fundamental Types of Hadron

 * `int` - signed integer, always 32 bits
 * `float` - floating point, 32 bits
 * `string` - string storage and basic operations (edited `String` class?), UTF-8 encoding
 * `char` - single character type, - how to deal with multi-byte UTF-8 code points?
 * `symbol` - immutable string type, (edited `Symbol` class?), UTF-8 encoding
 * `nil` - *needs thought*
 * `ClassName` - instance of `ClassName`

### Composite Types

The built-in low-level types `array` and `dict` require homogenous fundamental types for both keys and values. The
`Array`, `Dictionary`, and `IdentityDictionary` classes can all handle heterogenous values in storage.

### Type Decorators

We support two kinds of typed variable declaration, either appending the type of the variable at the end of the variable
name or by providing the type in place of the `arg` or `var` keywords.

Example code:

```
This {
     var value:int;
     calculate:int {
         |a:int, b:int, c:int|   // imaginary type hint....
         ^(a + b * c) * this.value
     }
}

That {
    int value;
    // return type too C-like? Also, type could be inferred.
    int calculate { int a, b, c;  // all times must be the same for this to work?
        ^(a + b * c) * this.value;
    }
}
```

### Inferred Types

Variables initialized to literals and function arguments with default values don't require type decorators. Other types
can be inferred by computation, say from operations on other elements with known types.

For example:

```
(
    var a = 5, k = "test";  // Both a and k have known type from their literal initalization, no type assignment needed.
    var b = a + 4; // Type of b trivially computable from type of a and literal.
    var c = a.toString() + k;
)
```
