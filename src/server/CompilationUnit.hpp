#ifndef SRC_SERVER_COMPILATION_UNIT_HPP_
#define SRC_SERVER_COMPILATION_UNIT_HPP_

#include <memory>
#include <string>

namespace hadron {
struct Frame;
struct LinearBlock;
class VirtualJIT;
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
    std::unique_ptr<hadron::VirtualJIT> virtualJIT;
};

} // namespace server

#endif // SRC_SERVER_COMPILATION_UNIT_HPP_