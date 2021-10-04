# Hadron Object System

Much of the design of the Object system in Hadron is contrainted by the existing design in LSC.

# At Compilation Time

Layout and primitives as member functions for all classes that contain primitives. This is to add some static analysis
to the primitive binding between .sc and .cpp files.

# At Class Library Compile Time

We need to know about Symbols, Classes, and Arrays ahead of time because all the other objects are defined in terms
of these.

Add a method to Interpreter `compileClass` which takes a parse::ClassNode* and adds an instance of `Class` to the
Class table, a map - for now both Symbol table and Class table can be maps.