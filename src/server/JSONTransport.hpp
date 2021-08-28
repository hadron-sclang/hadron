#ifndef SRC_SERVER_JSON_TRANSPORT_HPP_
#define SRC_SERVER_JSON_TRANSPORT_HPP_

#include "server/LSPTypes.hpp"

#include <stdio.h>

namespace server {

// Credit is due for this class to the authors clangd, which inspired much of the design and implementation of this
// class.
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