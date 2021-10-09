#include "hadron/ClassLibrary.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Heap.hpp"

namespace hadron {

ClassLibrary::ClassLibrary(std::shared_ptr<Heap> heap, std::shared_ptr<ErrorReporter> errorReporter):
    m_heap(heap), m_errorReporter(errorReporter) {}

void ClassLibrary::addClassFile(const std::string& classFile) {
    

}


} // namespace hadron