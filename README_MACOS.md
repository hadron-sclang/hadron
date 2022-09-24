** Building

Check out the code, then initialize the submodules:

`git submodule update --init --recursive`

Hadron uses [homebrew](https://brew.sh/) to install the required build dependencies. To build on MacOS requires the
following:

 * CMake
 * GNU Bison newer than 3.2
 * gperf (server dependency)
 * Ragel
 * ANTLR v4.11.1 or newer (for sparkler)

All of which can be installed with the command:

`brew install cmake bison gperf ragel antlr ninja`
