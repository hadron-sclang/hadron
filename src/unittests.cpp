#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

#include "spdlog/spdlog.h"

int main() {
    spdlog::set_level(spdlog::level::debug);
    doctest::Context context;
    int res = context.run();
    return res;
}
