#include "hadron/NameSaver.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/hir/AssignHIR.hpp"
#include "hadron/ThreadContext.hpp"

namespace hadron {

NameSaver::NameSaver(/*ThreadContext* context, */ std::shared_ptr<ErrorReporter> errorReporter):
    /* m_threadContext(context), */ m_errorReporter(errorReporter) {}

void NameSaver::scanFrame(Frame* frame) {
    std::unordered_set<Block::ID> visitedBlocks;
    scanBlock(frame->rootScope->blocks.front().get(), visitedBlocks);
}

void NameSaver::scanBlock(Block* block, std::unordered_set<Block::ID>& visitedBlocks) {
    visitedBlocks.emplace(block->id);

    auto iter = block->statements.begin();

    std::unordered_set<hir::ID> instanceValues;
    std::unordered_set<hir::ID> classValues;

    while (iter != block->statements.end()) {
        switch ((*iter)->opcode) {
        case hir::Opcode::kImportClassVariable:
            classValues.emplace((*iter)->id);
            break;

        case hir::Opcode::kImportInstanceVariable:
            instanceValues.emplace((*iter)->id);
            break;

        case hir::Opcode::kAssign: {
            auto assignHIR = reinterpret_cast<hir::AssignHIR*>(iter->get());
            auto stateIter = m_nameStates.find(assignHIR->name);
            // Is this a previously known state either update the value or remove the redundant assignment.
            if (stateIter != m_nameStates.end()) {
                if (stateIter->second.value == assignHIR->valueId) {
                    // Redundant assignment, remove.
                    block->frame->values[assignHIR->valueId]->consumers.erase(iter->get());
                    auto assignIter = block->nameAssignments.find(assignHIR->name);
                    assert(assignIter != block->nameAssignments.end());
                    if (assignIter->second == iter->get()) {
                        assignIter->second = stateIter->second.assign;
                    }
                    iter = block->statements.erase(iter);
                    continue;
                }

                // Update assignment to new value.
                stateIter->second.value = assignHIR->valueId;
                stateIter->second.assign = assignHIR;
                // TODO: actually emit instruction to save value if needed.
            } else {
                auto nameType = NameType::kLocal;
                int32_t index = -1;
                if (instanceValues.count(assignHIR->valueId)) {
                    nameType = NameType::kInstance;
                } else if (classValues.count(assignHIR->valueId)) {
                    nameType = NameType::kClass;
                }

                m_nameStates.emplace(std::make_pair(assignHIR->name, NameState{nameType, assignHIR->valueId, index,
                        assignHIR}));
            }
        } break;

        default:
            break;
        }

        ++iter;
    }

    for (auto succ : block->successors) {
        if (visitedBlocks.count(succ->id) == 0) {
            scanBlock(succ, visitedBlocks);
        }
    }
}

} // namespace hadron