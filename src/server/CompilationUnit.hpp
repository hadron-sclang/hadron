#ifndef SRC_SERVER_COMPILATION_UNIT_HPP_
#define SRC_SERVER_COMPILATION_UNIT_HPP_

#include <memory>
#include <string>

namespace hadron {
struct Frame;
struct LinearBlock;
namespace parse {
struct BlockNode;
} // namespace parse
} // namespace hadron

namespace server {

struct CompilationUnit {
    std::string name;
    const hadron::parse::BlockNode* blockNode;
    std::unique_ptr<hadron::Frame> frame;
    std::unique_ptr<hadron::LinearBlock> linearBlock;
    std::unique_ptr<int8_t[]> byteCode;
    size_t byteCodeSize;
};

} // namespace server

#endif // SRC_SERVER_COMPILATION_UNIT_HPP_