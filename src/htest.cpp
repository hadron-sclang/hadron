// htest, command line SuperCollider language script interpreter
#include "hadron/internal/FileSystem.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/SourceFile.hpp"

#include "fmt/format.h"
#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>
#include <regex>
#include <string_view>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    spdlog::default_logger()->set_level(spdlog::level::warn);

    if (argc != 2) {
        std::cerr << "usage: htest [options] input-file.sctest" << std::endl;
        return -1;
    }

    hadron::Runtime runtime;
    if (!runtime.initInterpreter()) { return -1; }

    fs::path sourcePath(argv[1]);
    auto sourceFile = hadron::SourceFile(sourcePath.string());
    if (!sourceFile.read()) { return -1; }

    auto tokenRegex = std::regex("(^|\\n)//[+][ ]*([A-Z]+):[ ]*([^ \\n]+[^\\n]*)?\\n");
    auto iter = std::cregex_iterator(sourceFile.code(), sourceFile.code() + sourceFile.size(), tokenRegex);
    auto endIter = std::cregex_iterator();

    std::string_view name;
    const char* payloadStart = nullptr;
    enum Verb {
        kExpecting,
        kNothing,
        kRun
    };
    Verb verb = kNothing;

    struct TestCommand {
        Verb verb = kNothing;
        std::string_view name;
        std::string_view payload;
    };

    std::vector<TestCommand> commands;

    for (; iter != endIter; ++iter) {
        auto& match = *iter;
        if (match.size() != 4) {
            std::cerr << "malformed test command string: " << match[0] << "\n";
            return -1;
        }

        // process whatever was already in the buffer as input to the last token
        if (payloadStart) {
            auto payload = std::string_view(payloadStart, match[0].first - payloadStart);
            commands.emplace_back(TestCommand{verb, name, payload});
        }

        name = std::string_view(match[3].first, match[3].second - match[3].first);
        payloadStart = match[0].second;

        if (match[2].compare("EXPECTING") == 0) {
            verb = kExpecting;
        } else if (match[2].compare("RUN") == 0) {
            verb = kRun;
        } else {
            std::cerr << "unknown test command: " << match[2] << "\n";
            return -1;
        }
    }

    if (payloadStart) {
        auto payload = std::string_view(payloadStart, sourceFile.code() + sourceFile.size() - payloadStart);
        commands.emplace_back(TestCommand{verb, name, payload});
    }

    std::string runResults;
    std::string_view runName;
    int errorCount = 0;

    // Execute the commands in the file, in order.
    for (const auto& command : commands) {
        switch(command.verb) {
        case kExpecting: {
            if (runResults != command.payload) {
                std::cerr << fmt::format("ERROR: running '{}', expected '{}' got '{}'\n", runName, command.payload,
                        runResults);
                ++errorCount;
            }
        } break;

        case kNothing:
            std::cerr << "encountered nothing command after parsing\n";
            return -1;

        case kRun: {
            auto slotResult = runtime.interpret(command.payload);
            runResults = runtime.slotToString(slotResult);
            runName = command.name;
        } break;
        }
    }

    if (errorCount > 0) {
        std::cerr << "errors in test file " << sourcePath << std::endl;
        return -1;
    }

    return 0;
}