SCLang To LLVM Concepts Mapping
-------------------------------

Hadron seeks to generate LLVM IR, intermediate representation bytecode from sclang programming. This LLVM IR can then
be optimized by LLVM and converted to machine-specific bytecode for fast execution.

The LLVM [Module](https://llvm.org/doxygen/classllvm_1_1Module.html) is intended to map to a single file of program
code. It makes sense that each SuperCollider Class would require its own Module.

For interpreted code we generate a new Module with a `main` entry point and emit the bytecode there.

