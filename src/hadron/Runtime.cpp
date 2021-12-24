#include "hadron/Runtime.hpp"

#include "hadron/ClassLibrary.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Heap.hpp"
#include "hadron/ThreadContext.hpp"
#include "internal/FileSystem.hpp"

#include "spdlog/spdlog.h"

#include <array>

namespace {
static const std::array<const char*, 1> kClassFileAllowList{
//    "Kernel.sc",
    "Object.sc"
};
}

namespace hadron {

Runtime::Runtime(std::shared_ptr<ErrorReporter> errorReporter):
    m_errorReporter(errorReporter),
    m_heap(std::make_shared<Heap>()),
    m_threadContext(std::make_unique<ThreadContext>()),
    m_classLibrary(std::make_unique<ClassLibrary>(errorReporter)) {
    m_threadContext->heap = m_heap;
}

Runtime::~Runtime() {}

bool Runtime::initialize() {
    if (!recompileClassLibrary()) return false;

    return true;
}

bool Runtime::recompileClassLibrary() {
    auto classLibPath = findSCClassLibrary();
    SPDLOG_INFO("Starting Class Library compilation for files at {}", classLibPath.c_str());

    for (auto& entry : fs::recursive_directory_iterator(classLibPath)) {
        const auto& path = entry.path();
        if (!fs::is_regular_file(path) || path.extension() != ".sc")
            continue;
        // For now we have an allowlist in place, to control which SC class files are parsed, and we lazily do an
        // O(n) search in the array for each one.
        bool classFileAllowed = false;
        for (size_t i = 0; i < kClassFileAllowList.size(); ++i) {
            if (path.filename().compare(kClassFileAllowList[i]) == 0) {
                classFileAllowed = true;
                break;
            }
        }
        if (!classFileAllowed) {
            // SPDLOG_WARN("Skipping compilation of {}, not in class file allow list.", path.c_str());
            continue;
        }

        SPDLOG_INFO("Class Library compiling '{}'", path.c_str());
        m_classLibrary->addClassFile(m_threadContext.get(), path);
    }
    return true;
}

} // namespace hadron