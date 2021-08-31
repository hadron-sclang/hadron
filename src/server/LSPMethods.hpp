#ifndef SRC_SERVER_LSP_METHODS_HPP_
#define SRC_SERVER_LSP_METHODS_HPP_

#include <cstddef>

namespace server {
namespace lsp {

enum Method {
    kNotFound,      // no match
    kInitialize,
    kInitialized,
    kShutdown,
    kExit,
    kLogTrace,
    kSetTrace,

    // Hadron-specific extensions in the 'hadron/' method namespace
    kHadronParseTree,
    kHadronBlockFlow,
    kHadronLinearBlock,
    kHadronMachineCode
};

Method getMethodNamed(const char* name, size_t length);

} // namespace lsp
} // namespace server

#endif // SRC_SERVER_LSP_METHODS_HPP_