#include "Lexer.hpp"

#include <gflags/gflags.h>
#include <spdlog/spdlog.h>

DEFINE_string(lexerStateMachineFile, "", "If defined, a .dot file to save the lexer state machine to.");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    spdlog::set_level(spdlog::level::debug);

    if (FLAGS_lexerStateMachineFile.size()) {
        hadron::Lexer::saveLexerStateMachineGraph(FLAGS_lexerStateMachineFile.data());
    }

    return 0;
}
