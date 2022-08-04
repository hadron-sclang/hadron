%%{
    machine parser;

    action startName { name = p; }

    main := |*
        '// CLASSES:' space* (any - '\n' %startName)* '\n' {
            std::cout << "classes\n" << std::string_view(ts, te - ts);
            name = nullptr;
         };
        '// RUN:' space* (any - '\n' %startName)* '\n' {
            auto runName = name ? std::string_view(name, te - name - 1) : "nada";
            std::cout << fmt::format("run: '{}'\n", runName);
            name = nullptr;
        };
        '// EXPECTING:' space* (any - '\n')+ '\n' { std::cout << "expecting\n"; };
        '// ERROR:' space* (any - '\n')+ '\n' { std::cout << "error\n"; };
        '// WARN:' space* (any - '\n')+ '\n' { std::cout << "warn\n"; };

        any { };
    *|;
}%%

#include "hadron/ErrorReporter.hpp"
#include "hadron/internal/FileSystem.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/SourceFile.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>
#include <string_view>
#include <string>

namespace {
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-const-variable"
    %% write data;
#   pragma GCC diagnostic pop
}

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    spdlog::default_logger()->set_level(spdlog::level::trace);

    if (argc != 2) {
        std::cerr << "usage: hlang-test [options] input-file.sctest" << std::endl;
        return -1;
    }

    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    hadron::Runtime runtime(errorReporter);
    if (!runtime.initInterpreter()) { return -1; }

    fs::path sourcePath(argv[1]);
    auto sourceFile = hadron::SourceFile(sourcePath.string());
    if (!sourceFile.read(errorReporter)) { return -1; }


    {
        const char* p = sourceFile.code();
        const char* pe = sourceFile.code() + sourceFile.size();
        const char* eof = pe;
        int cs;
        int act;
        const char* ts;
        const char* te;

        const char* name = nullptr;

        %% write init;
        %% write exec;
    }

    return 0;
}