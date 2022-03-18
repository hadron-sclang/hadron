#include "hadron/NameSaver.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/ThreadContext.hpp"

namespace hadron {

NameSaver::NameSaver(ThreadContext* context, std::shared_ptr<ErrorReporter> errorReporter):
    m_threadContext(context), m_errorReporter(errorReporter) {}

void NameSaver::scanFrame(Frame* frame) {
    std::unordered_set<Block::ID> visitedBlocks;
    scanBlock(frame->rootScope->blocks.front().get(), visitedBlocks);
}

void NameSaver::scanBlock(Block* block, std::unordered_set<Block::ID>& visitedBlocks) {

}

} // namespace hadron