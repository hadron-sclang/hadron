#ifndef SRC_COMPILER_INCLUDE_HADRON_SOURCE_FILE_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_SOURCE_FILE_HPP_

#include <memory>
#include <string>
#include <string_view>

namespace hadron {

// Represents a file of source code. Inserts a null character at the end of the loaded string, for ease of use when
// parsing/handling.
class SourceFile {
public:
    SourceFile() = delete;
    SourceFile(std::string path);
    ~SourceFile() = default;

    bool read();

    const char* code() const { return m_code.get(); }
    size_t size() const { return m_codeSize; }
    std::string_view codeView() const { return std::string_view(m_code.get(), m_codeSize); }

private:
    std::string m_path;
    size_t m_codeSize;
    std::unique_ptr<char[]> m_code;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_SOURCE_FILE_HPP_