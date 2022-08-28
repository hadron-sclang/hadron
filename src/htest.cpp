// htest, command line SuperCollider language script interpreter
#include "hadron/ClassLibrary.hpp"
#include "hadron/internal/FileSystem.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/SlotDumpJSON.hpp"
#include "hadron/SlotDumpJSON.hpp"
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

DEFINE_bool(dumpClassArray, false, "After finalizing, dump class array to JSON");

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

    auto tokenRegex = std::regex("(^|\\n)//[+][ ]*([/A-Z]+):[ ]*([^ \\n]+[^\\n]*)?");
    auto iter = std::cregex_iterator(sourceFile.code(), sourceFile.code() + sourceFile.size(), tokenRegex);
    auto endIter = std::cregex_iterator();

    std::string_view name;
    const char* payloadStart = nullptr;
    enum Verb {
        kCheck,
        kClasses,
        kExpecting,
        kGives,
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
        if (verb != kNothing) {
            auto payload = std::string_view(payloadStart, match[0].first - payloadStart);

//            std::string verbName = verb == kExpecting ? "expect" : "run";
//            std::cout << fmt::format("verb: '{}', name: '{}', payload: '{}'\n", verbName, name, payload);

            commands.emplace_back(TestCommand{verb, name, payload});
        }

        name = std::string_view(match[3].first, match[3].second - match[3].first);
        // Payload starts after the newline at the end of the match.
        payloadStart = match[0].second + 1;

        if (match[2].compare("CHECK") == 0) {
            verb = kCheck;
        } else if (match[2].compare("CLASSES") == 0) {
            verb = kClasses;
        } else if (match[2].compare("EXPECTING") == 0) {
            verb = kExpecting;
        } else if (match[2].compare("GIVES") == 0) {
            verb = kGives;
        } else if (match[2].compare("RUN") == 0) {
            verb = kRun;
        } else if (match[2].compare("//") == 0) {
            verb = kNothing;
        } else {
            std::cerr << "unknown test command: " << match[2] << "\n";
            return -1;
        }
    }

    if (payloadStart) {
        auto payload = std::string_view(payloadStart, sourceFile.code() + sourceFile.size() - payloadStart - 1);
        commands.emplace_back(TestCommand{verb, name, payload});
    }

    std::string runResults;
    std::string_view runName;
    int errorCount = 0;
    bool finalizedLibrary = false;

    // Execute the commands in the file, in order.
    for (const auto& command : commands) {
        switch(command.verb) {
        case kCheck: {
            if (!finalizedLibrary) {
                if (FLAGS_dumpClassArray) {
                    auto dump = hadron::SlotDumpJSON();
                    dump.dump(runtime.context(), runtime.context()->classLibrary->classArray().slot(), true);
                    std::cout << dump.json() << std::endl;
                }
                finalizedLibrary = true;
            }
            auto slotResult = runtime.interpret(command.name);
            runResults = runtime.slotToString(slotResult);
            runName = command.name;
        } break;

        case kClasses: {
            if (!runtime.scanClassString(command.payload, sourcePath.string())) {
                std::cerr << "failed to scan class input string.\n";
                ++errorCount;
            }
        } break;

        case kExpecting: {
            if (runResults != command.payload) {
                std::cerr << fmt::format("ERROR: running '{}', expected '{}' got '{}'\n", runName, command.payload,
                        runResults);
                ++errorCount;
            }
        } break;

        case kGives: {
            if (runResults != command.name) {
                std::cerr << fmt::format("ERROR: running '{}', expected '{}' got '{}'\n", runName, command.name,
                        runResults);
                ++errorCount;
            }
        } break;

        case kNothing: // no-op
            break;

        case kRun: {
            if (!finalizedLibrary) {
                runtime.finalizeClassLibrary();
                if (FLAGS_dumpClassArray) {
                    auto dump = hadron::SlotDumpJSON();
                    dump.dump(runtime.context(), runtime.context()->classLibrary->classArray().slot(), true);
                    std::cout << dump.json() << std::endl;
                }
                finalizedLibrary = true;
            }
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