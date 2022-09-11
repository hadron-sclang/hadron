// prim, parses a SuperCollider input class file and produces primitive function signatures and routing fragments
#include "hadron/SourceFile.hpp"

#include "fmt/format.h"
#include "gflags/gflags.h"
#include "SCLexer.h"
#include "SCParser.h"
#include "SCParserBaseListener.h"
#include "SCParserListener.h"

namespace {

class PrimitiveListener : public sprklr::SCParserBaseListener {
public:
    PrimitiveListener() {}
    virtual ~PrimitiveListener() {}


};

} // namespace

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (argc != 2) {
        std::cerr << "usage: prim [options] input-file.sc\n";
        return -1;
    }


    return 0;
}