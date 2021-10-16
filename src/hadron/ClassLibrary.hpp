#ifndef SRC_HADRON_CLASS_LIBRARY_HPP_
#define SRC_HADRON_CLASS_LIBRARY_HPP_

#include <memory>
#include <string>

namespace hadron {

class ErrorReporter;
class Heap;

class ClassLibrary {
public:
    ClassLibrary(std::shared_ptr<Heap> heap, std::shared_ptr<ErrorReporter> errorReporter);
    ClassLibrary() = delete;
    ~ClassLibrary() = default;

    // Parse and compile a class file, then add it to the Heap.
    bool addClassFile(const std::string& classFile);

private:
    std::shared_ptr<Heap> m_heap;
    std::shared_ptr<ErrorReporter> m_errorReporter;
};

} // namespace hadron

#endif // SRC_HADRON_CLASS_LIBRARY_HPP_