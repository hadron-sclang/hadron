#ifndef SRC_COMPILER_INCLUDE_HADRON_SOURCE_FILE_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_SOURCE_FILE_HPP_

#include <string>
#include <string_view>

namespace hadron {

class ErrorReporter;

// Represents a file of source code.
class SourceFile {
public:
    SourceFile() = delete;
    SourceFile(std::string path);
    ~SourceFile() = default;

    bool read(std::shared_ptr<ErrorReporter> errorReporter);

    const char* code() const { return m_code.get(); }
    size_t size() const { return m_codeSize; }
    std::string_view codeView() const { return std::string_view(m_code.get(), m_codeSize); }
private:
    std::string m_path;
    size_t m_codeSize;
    std::unique_ptr<char[]> m_code;
};

} // namespace hadron

#endif  // SRC_COMPILER_INCLUDE_HADRON_SOURCE_FILE_HPP_