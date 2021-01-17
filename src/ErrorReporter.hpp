#ifndef SRC_ERROR_REPORTER_HPP_
#define SRC_ERROR_REPORTER_HPP_

#include <string>
#include <vector>

namespace hadron {

class ErrorReporter {
public:
    ErrorReporter();
    ~ErrorReporter();

    // Most be called before getLineNumber() can be called.
    void setCode(const char* code) { m_code = code; }

    void addError(const std::string& error);

    size_t getLineNumber(const char* location);
    size_t errorCount() { return m_errors.size(); }

private:
    const char* m_code;
    std::vector<std::string> m_errors;
    std::vector<const char*> m_lineEndings;
};

} // namespace hadron

#endif // SRC_ERROR_REPORTER_HPP_
