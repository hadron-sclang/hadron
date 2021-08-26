// hlangd is the Hadron Language Server, which communicates via JSON-RPCv2 via stdin/stdout.

#include "gflags/gflags.h"

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);


    return 0;
}