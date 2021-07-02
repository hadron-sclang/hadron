#include "ErrorReporter.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

ErrorReporter::ErrorReporter(bool suppress): m_suppress(suppress), m_code(nullptr) {}

ErrorReporter::~ErrorReporter() {}

void ErrorReporter::addError(const std::string& error) {
    if (!m_suppress) {
        spdlog::error(error);
    }
    m_errors.emplace_back(error);
}

size_t ErrorReporter::getLineNumber(const char* location) {
    // Lazily construct the line number map on first request for line number.
    if (!m_lineEndings.size()) {
        const char* code = m_code;
        m_lineEndings.emplace_back(code);
        while (*code != '\0') {
            if (*code == '\n') {
                m_lineEndings.emplace_back(code);
            }
            ++code;
        }
        m_lineEndings.emplace_back(code);
    }

    // Binary search on ranges to find line number.
    size_t start = 0;
    size_t end = m_lineEndings.size() - 1;
    while (start < end) {
        size_t middle = start + ((end - start) / 2);
        if (m_lineEndings[middle] < location) {
            if (m_lineEndings[middle + 1] >= location) {
                return middle + 1;
            }
            start = middle;
        } else {
            end = middle;
        }
    }

    return start + 1;
}

} // namespace hadron
