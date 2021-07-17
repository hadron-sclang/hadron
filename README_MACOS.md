** Building

Check out the code, then initialize the submodules:

`git submodule update --init --recursive`

Hadron uses [homebrew](https://brew.sh/) to install the required build dependencies. To build on MacOS requires the
following:

 * CMake
 * GNU Autotools, specifically automake and autoconf
 * GNU Libtool

All of which can be installed with the command:

`brew install cmake automake autoconf libtool`

