#ifndef SRC_ERROR_REPORTER_HPP_
#define SRC_ERROR_REPORTER_HPP_

#include <string>
#include <vector>

namespace hadron {

class ErrorReporter {
public:
    // If suppress is true, will not print reported errors to log (useful for testing failures without
    // polluting the log)
    ErrorReporter(bool suppress = false);
    ~ErrorReporter();

    // Must be called before getLineNumber() can be called.
    void setCode(const char* code) { m_code = code; }

    void addError(const std::string& error);

    // Specific errors.

    // Fatal error, unable to locate a file under filePath.
    void addFileNotFoundError(std::string filePath);
    // Fatal error, unable to open file at filePath.
    void addFileOpenError(std::string filePath);
    // Fatal compiler error, failed to read file at filePath.
    void addFileReadError(std::string filePath);

    size_t getLineNumber(const char* location);
    size_t errorCount() const { return m_errors.size(); }
    bool ok() const { return m_errors.size() == 0; }
    const char* getLineStart(size_t lineNumber) const { return m_lineEndings[lineNumber - 1]; }

private:
    bool m_suppress;
    const char* m_code;
    std::vector<std::string> m_errors;
    std::vector<const char*> m_lineEndings;
};

} // namespace hadron

#endif // SRC_ERROR_REPORTER_HPP_
