# Hadron Object System

Much of the design of the Object system in Hadron is contrainted by the existing design in LSC.

# At Compilation Time

Layout and primitives as member functions for all classes that contain primitives. This is to add some static analysis
to the primitive binding between .sc and .cpp files.

# At Class Library Compile Time

We need to know about Symbols, Classes, and Arrays ahead of time because all the other objects are defined in terms
of these.

Add a method to Interpreter `compileClass` which takes a `parse::ClassNode*` and adds an instance of `Class` to the
Class table, a map - for now both Symbol table and Class table can be maps. So, the job of the `compileClass` function
is to create an *instance* of Class with name `Foo` - but where to class variables and methods live?

```
Object.class -> Meta_Object

Object.dump ->

class Object (0x7ffc603f8280) {
  instance variables [19]
    name : Symbol 'Object'
    nextclass : instance of Meta_ObjectGui (0x7ffc402ecd80, size=19, set=5)
    superclass : Symbol '__none'
    subclasses : instance of Array (0x7ffc40c4c8c0, size=201, set=8)
    methods : instance of Array (0x7ffc40be1980, size=270, set=9)
    instVarNames : nil
    classVarNames : instance of SymbolArray (0x7ffc603f8180, size=4, set=2)
    iprototype : nil
    cprototype : instance of Array (0x7ffc603f8400, size=4, set=2)
    constNames : instance of SymbolArray (0x7ffc603f84c0, size=1, set=2)
    constValues : instance of Array (0x7ffc5024ee80, size=1, set=2)
    instanceFormat : Integer 0
    instanceFlags : Integer 0
    classIndex : Integer 0
    classFlags : Integer 1
    maxSubclassIndex : Integer 2251
    filenameSymbol : Symbol '/Applications/SuperCollider.app/Contents/Resources/SCClassLibrary/Common/Core/Object.sc'
    charPos : Integer 0
    classVarIndex : Integer 0
}
```

```
Meta_Object.class -> Class

Meta_Object.dump ->

class Meta_Object (0x7ffc603f7d00) {
  instance variables [19]
    name : Symbol 'Meta_Object'
    nextclass : class Meta_ObjectGui (0x7ffc402ec740)
    superclass : Symbol 'Class'
    subclasses : instance of Array (0x7ffc40c4d940, size=201, set=8)
    methods : instance of Array (0x7ffc603f7e80, size=11, set=4)
    instVarNames : instance of SymbolArray (0x7ffc603f8000, size=19, set=4)
    classVarNames : nil
    iprototype : instance of Array (0x7ffc5024db80, size=19, set=5)
    cprototype : nil
    constNames : nil
    constValues : nil
    instanceFormat : Integer 0
    instanceFlags : Integer 0
    classIndex : Integer 1126
    classFlags : Integer 1
    maxSubclassIndex : Integer 2251
    filenameSymbol : Symbol '/Applications/SuperCollider.app/Contents/Resources/SCClassLibrary/Common/Core/Object.sc'
    charPos : Integer 0
    classVarIndex : Integer 0
}
```

```
Class.class -> Meta_Class

Class.dump ->

class Class (0x7ffc603f8b00) {
  instance variables [19]
    name : Symbol 'Class'
    nextclass : instance of Meta_ClassBrowser (0x7ffc40b19900, size=19, set=5)
    superclass : Symbol 'Object'
    subclasses : instance of Array (0x7ffc482273c0, size=1, set=2)
    methods : instance of Array (0x7ffc603f8c80, size=49, set=6)
    instVarNames : instance of SymbolArray (0x7ffc603f8980, size=19, set=4)
    classVarNames : instance of SymbolArray (0x7ffc603f9100, size=1, set=2)
    iprototype : instance of Array (0x7ffc408e9840, size=19, set=5)
    cprototype : instance of Array (0x7ffc408e9ac0, size=1, set=2)
    constNames : nil
    constValues : nil
    instanceFormat : Integer 0
    instanceFlags : Integer 0
    classIndex : Integer 1125
    classFlags : Integer 1
    maxSubclassIndex : Integer 2251
    filenameSymbol : Symbol '/Applications/SuperCollider.app/Contents/Resources/SCClassLibrary/Common/Core/Kernel.sc'
    charPos : Integer 254
    classVarIndex : Integer 4
}
```

```
Meta_Class.class -> Class

Meta_Class.dump ->

class Meta_Class (0x7ffc603f8580) {
  instance variables [19]
    name : Symbol 'Meta_Class'
    nextclass : class Meta_ClassBrowser (0x7ffc40b192c0)
    superclass : Symbol 'Meta_Object'
    subclasses : nil
    methods : instance of Array (0x7ffc603f8700, size=5, set=3)
    instVarNames : instance of SymbolArray (0x7ffc603f8800, size=19, set=4)
    classVarNames : nil
    iprototype : instance of Array (0x7ffc408e95c0, size=19, set=5)
    cprototype : nil
    constNames : nil
    constValues : nil
    instanceFormat : Integer 0
    instanceFlags : Integer 0
    classIndex : Integer 2251
    classFlags : Integer 1
    maxSubclassIndex : Integer 2251
    filenameSymbol : Symbol '/Applications/SuperCollider.app/Contents/Resources/SCClassLibrary/Common/Core/Kernel.sc'
    charPos : Integer 254
    classVarIndex : Integer 4
}
```
