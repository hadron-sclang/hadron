// hlangd is the Hadron Language Server, which communicates via JSON-RPCv2 via stdin/stdout.
#include "hadron/Interpreter.hpp"
#include "server/JSONTransport.hpp"

#include "gflags/gflags.h"

#include <stdio.h>

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    server::JSONTransport jsonTransport(stdin, stdout);
    int returnCode = jsonTransport.runLoop();

    return returnCode;
}