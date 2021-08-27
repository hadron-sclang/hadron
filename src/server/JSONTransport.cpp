#include "JSONTransport.hpp"

#include <array>
#include <string>

namespace server {

JSONTransport::JSONTransport(FILE* inputStream, FILE* outputStream):
    m_inputStream(inputStream),
    m_outputStream(outputStream) {}

int JSONTransport::runLoop() {
    std::array<char, 128> headerLine;
    int headerSize;

    while (!feof(m_inputStream)) {
        if (ferror(m_inputStream)) {
            return -1;
        }
        // parse headers
        // parse input json
        headerSize = fread(headerLine.data(), 1, sizeof(headerLine), m_inputStream);
    }
    fwrite(headerLine.data(), 1, headerSize, m_outputStream);

    // EOF means normal exit of loop, return.
    return 0;
}

} // namespace server