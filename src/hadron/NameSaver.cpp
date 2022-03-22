#include "hadron/NameSaver.hpp"

#include "hadron/Block.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/hir/AssignHIR.hpp"
#include "hadron/hir/BlockLiteralHIR.hpp"
#include "hadron/Scope.hpp"
#include "hadron/ThreadContext.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

NameSaver::NameSaver(ThreadContext* context, std::shared_ptr<ErrorReporter> errorReporter):
    m_threadContext(context), m_errorReporter(errorReporter) {}

void NameSaver::scanFrame(Frame* frame) {
    // We start by scanning the import block in the top-level frame, which will have accurate types for all imported
    // values. Nested frames import everything but arguments as local variables, because they are defined locally in the
    // outer scope.
    std::unordered_map<hir::ID, NameType> valueTypes;

    auto iter = frame->rootScope->blocks.front()->statements().begin();
    while (iter != frame->rootScope->blocks.front()->statements().end()) {
        switch ((*iter)->opcode) {
        case hir::Opcode::kImportClassVariable:
            valueTypes.emplace(std::make_pair((*iter)->id, NameType::kClass));
            break;

        case hir::Opcode::kImportInstanceVariable:
            valueTypes.emplace(std::make_pair((*iter)->id, NameType::kInstance));
            break;

        case hir::kLoadArgument:
        case hir::kBranch:
            break;

        case hir::Opcode::kAssign: {
            auto assignHIR = reinterpret_cast<hir::AssignHIR*>(iter->get());
            // Names in the import block are assumed to be unique and assigned only once.
            assert(m_nameStates.find(assignHIR->name) == m_nameStates.end());

            // If this is a class or instance variable value, create the mapping.
            auto valueIter = valueTypes.find(assignHIR->valueId);
            if (valueIter != valueTypes.end()) {
            m_nameStates.emplace(std::make_pair(assignHIR->name, NameState{valueIter->second, assignHIR->valueId,
                    assignHIR->valueId, -1, assignHIR}));
            }

        } break;

        default:
            // Import of class and instance variables, load arguments, assigns, and the branch to the next block are the
            // only statements that should appear in the top-level import block.
            assert(false);
        }

        break;
    }

    // We now have the name of every class and instance variable this frame modifies, so on subsequent AssignHIR values
    // referring to these names we can insert HIR to save the new value out to appropriate spot, if necessary.

    // ** We need a *writeName* and a *readName* HIR instead of just assign - an *assign* is an association of a name
    // with a certain value at a certain point in time. The problem with captured values is that we can't assume they
    // are stable outside of the method code. So that means that not only do we need to flush the writes out on any
    // return, but on *any message call*

    // What if every variable, like every argument, had a backing store, and the interpreter just consistently flushed
    // writes, and always re-read the value from the store on re-read?

    // Can just scan frame->values[] for inlineblocks, saving the graph traversal, and then only need to scan the import
    // block of each subframe for the import statements.

    std::unordered_set<Block::ID> visitedBlocks;
    scanBlock(frame->rootScope->blocks.front().get(), visitedBlocks);
}

void NameSaver::scanBlock(Block* block, std::unordered_set<Block::ID>& visitedBlocks) {
    visitedBlocks.emplace(block->id());


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