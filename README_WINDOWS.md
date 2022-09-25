You will need the MSVC toolchain and a Java runtime for ANTLR4. Probably best to look
at how the build bots configure ANTLR4 and Bison, which are not provided by vcpkg.

Recommend using vcpkg for build-time dependencies.

vcpkg install --triplet x64-windows ragel gperf
