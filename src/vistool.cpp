#include <gflags/gflags.h>
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    spdlog::set_level(spdlog::level::debug);

    return 0;
}
