#ifndef SRC_SERVER_LSP_TYPES_HPP_
#define SRC_SERVER_LSP_TYPES_HPP_

#include <string>
#include <variant>

namespace server {
namespace lsp {

using ID = std::variant<int64_t, std::string>;

} // namespace lsp
} // namespace server

#endif // SRC_SERVER_LSP_TYPES_HPP_