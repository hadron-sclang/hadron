#include "hadron/Runtime.hpp"

#include "hadron/ClassLibrary.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Heap.hpp"
#include "hadron/ThreadContext.hpp"

namespace hadron {

Runtime::Runtime(std::shared_ptr<ErrorReporter> errorReporter):
    m_errorReporter(errorReporter),
    m_heap(std::make_shared<Heap>()),
    m_threadContext(std::make_unique<ThreadContext>()),
    m_classLibrary(std::make_unique<ClassLibrary>(m_heap, errorReporter)) {}

Runtime::~Runtime() {}

bool Runtime::initialize() {

    return true;
}

// Need to find the built-in class library. LSC uses the SC_Filesystem to find the Resource directory and expects
// SCClassLibrary to be installed relative to that. For now we have a os-specific means of finding the path to the
// currently running binary, and then expect it to be relative to that. Move over to Filesystem.hpp and take from there.
void Runtime::findRunningBinary() {


}

} // namespace hadron