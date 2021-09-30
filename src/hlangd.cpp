// hlangd is the Hadron Language Server, which communicates via JSON-RPCv2 via stdin/stdout.
#include "hadron/internal/BuildInfo.hpp"
#include "hadron/Interpreter.hpp"
#include "server/HadronServer.hpp"
#include "server/JSONTransport.hpp"

#include "gflags/gflags.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

#include <memory>
#include <stdio.h>

DEFINE_string(logFile, "hlangdLog.txt", "Path and file name of log file.");
DEFINE_bool(debugLogs, false, "Set log output level to debug (verbose).");
DEFINE_bool(traceLogs, true, "Set log output level to trace (very verbose).");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    auto logger = spdlog::basic_logger_mt("file", FLAGS_logFile);
    logger->flush_on(spdlog::level::info);
    if (FLAGS_debugLogs) {
        logger->set_level(spdlog::level::level_enum::debug);
        logger->flush_on(spdlog::level::debug);
    }
    if (FLAGS_traceLogs) {
        logger->set_level(spdlog::level::level_enum::trace);
        logger->flush_on(spdlog::level::trace);
    }
    spdlog::set_default_logger(logger);
    SPDLOG_INFO("Hadron version {}, git branch {}@{}, compiled by {} version {}.", hadron::kHadronVersion,
        hadron::kHadronBranch, hadron::kHadronCommitHash, hadron::kHadronCompilerName, hadron::kHadronCompilerVersion);

    auto transport = std::make_unique<server::JSONTransport>(stdin, stdout);
    server::HadronServer server(std::move(transport));

    int returnCode = server.runLoop();

    return returnCode;
}