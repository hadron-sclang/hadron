#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

#include "spdlog/spdlog.h"

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::debug);
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    int res = context.run();
    return res;
}
