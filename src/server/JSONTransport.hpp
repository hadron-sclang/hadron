#ifndef SRC_SERVER_JSON_TRANSPORT_HPP_
#define SRC_SERVER_JSON_TRANSPORT_HPP_

#include <stdio.h>

namespace server {

class JSONTransport {
public:
    // Non-owning input pointers. These are using the stdio input mechanisms because of some documentation in the clangd
    // source code that indicates compatibility and reliability issues on the input stream when using C++ stdlib stream
    // interfaces.
    JSONTransport(FILE* inputStream, FILE* outputStream);

    // The main run loop, returns an exit status code.
    int runLoop();

private:
    FILE* m_inputStream;
    FILE* m_outputStream;
};

} // namespace server

#endif // SRC_SERVER_JSON_TRANSPORT_HPP_