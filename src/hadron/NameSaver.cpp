#include "hadron/NameSaver.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/hir/AssignHIR.hpp"
#include "hadron/hir/BlockLiteralHIR.hpp"
#include "hadron/ThreadContext.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

NameSaver::NameSaver(ThreadContext* context, std::shared_ptr<ErrorReporter> errorReporter):
    m_threadContext(context), m_errorReporter(errorReporter) {}

void NameSaver::scanFrame(Frame* frame) {
    std::unordered_set<Block::ID> visitedBlocks;
    scanBlock(frame->rootScope->blocks.front().get(), visitedBlocks);
}

void NameSaver::scanBlock(Block* block, std::unordered_set<Block::ID>& visitedBlocks) {
    visitedBlocks.emplace(block->id());

    std::unordered_map<hir::ID, NameType> valueTypes;

    auto iter = block->statements().begin();
    while (iter != block->statements().end()) {
        switch ((*iter)->opcode) {
        case hir::Opcode::kBlockLiteral: {
            auto blockHIR = reinterpret_cast<hir::BlockLiteralHIR*>(iter->get());
            NameSaver saver(m_threadContext, m_errorReporter);
            saver.scanFrame(blockHIR->frame.get());
        } break;

        case hir::Opcode::kImportClassVariable:
            valueTypes.emplace(std::make_pair((*iter)->id, NameType::kClass));
            break;

        case hir::Opcode::kImportInstanceVariable:
            valueTypes.emplace(std::make_pair((*iter)->id, NameType::kInstance));
            break;

        case hir::Opcode::kImportLocalVariable:
            valueTypes.emplace(std::make_pair((*iter)->id, NameType::kExternal));
            break;

        case hir::Opcode::kAssign: {
            auto assignHIR = reinterpret_cast<hir::AssignHIR*>(iter->get());
            auto stateIter = m_nameStates.find(assignHIR->name);
            // Is this a previously known state either update the value or remove the redundant assignment.
            if (stateIter != m_nameStates.end()) {
                if (stateIter->second.value == assignHIR->valueId) {
                    // Redundant assignment, remove.
                    block->frame()->values[assignHIR->valueId]->consumers.erase(iter->get());
                    auto assignIter = block->nameAssignments().find(assignHIR->name);
                    assert(assignIter != block->nameAssignments().end());
                    if (assignIter->second == iter->get()) {
                        assignIter->second = stateIter->second.assign;
                    }
                    iter = block->statements().erase(iter);
                    continue;
                }

                // Update assignment to new value.
                stateIter->second.value = assignHIR->valueId;
                stateIter->second.assign = assignHIR;

                // TODO: actually emit instruction to save value if needed.
                if (stateIter->second.type == NameType::kExternal &&
                        stateIter->second.initialValue != stateIter->second.value) {
                    SPDLOG_CRITICAL("capture needed for {}", stateIter->first.view(m_threadContext));
                }
            } else {
                // Create new value with this name, no save needed as this is initial load.
                auto nameType = NameType::kLocal;
                auto valueIter = valueTypes.find(assignHIR->valueId);
                if (valueIter != valueTypes.end()) {
                    nameType = valueIter->second;
                }

                m_nameStates.emplace(std::make_pair(assignHIR->name, NameState{nameType, assignHIR->valueId,
                        assignHIR->valueId, -1, assignHIR}));
            }
        } break;

        default:
            break;
        }

        ++iter;
    }

    for (auto succ : block->successors()) {
        if (visitedBlocks.count(succ->id()) == 0) {
            scanBlock(succ, visitedBlocks);
        }
    }
}

} // namespace hadron