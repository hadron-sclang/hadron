#include "hadron/LightningJIT.hpp"

#include "hadron/Hash.hpp"
#include "hadron/SyntaxAnalyzer.hpp"
#include "Keywords.hpp"

#include "spdlog/spdlog.h"

extern "C" {
#include "lightning.h"
}

namespace hadron {

class LightningJITBlock : public JITBlock {
public:
    LightningJITBlock(): m_blockEval(nullptr) {
        m_jitState = jit_new_state();
    }

    virtual ~LightningJITBlock() {
        _jit_destroy_state(m_jitState);
    }

    typedef int (*BlockEval)(void);

    int value() override {
        return m_blockEval();
    }

    void printJIT() override {
        _jit_print(m_jitState);
        _jit_clear_state(m_jitState);
    }

    // idea: can add opcode methods here like movr(), jmp() etc, instead of exposing jitState()
    jit_state_t* jitState() { return m_jitState; }


    bool emit() {
        m_blockEval = reinterpret_cast<BlockEval>(_jit_emit(m_jitState));
        return m_blockEval != nullptr;
    }

private:
    jit_state_t* m_jitState;
    BlockEval m_blockEval;
};

struct RegisterAllocator {
    // For Fibonacci only we can just use the V registers and hard-allocate them at the high block level. This will
    // need to get much more sophisticated to survive anything beyond Fibonacci. For now this is a simple map from
    // Value nameHash to register number.
    std::unordered_map<Hash, int> map;
};

} // namespace hadron


namespace {
void jitAST(const hadron::ast::AST* ast, hadron::LightningJITBlock* jit, hadron::RegisterAllocator* allocator) {
    switch (ast->astType) {
    // no type checking, none at all
    case hadron::ast::ASTType::kAssign: {
        const auto assign = reinterpret_cast<const hadron::ast::AssignAST*>(ast);
        int targetReg = allocator->map.find(assign->target->nameHash)->second;
        if (assign->value->astType == hadron::ast::ASTType::kValue) {
            const auto target = reinterpret_cast<const hadron::ast::ValueAST*>(assign->value.get());
            int valueReg = allocator->map.find(target->nameHash)->second;
            // movr %targetReg, %valueReg
            _jit_new_node_ww(jit->jitState(), jit_code_movr, JIT_V(targetReg), JIT_V(valueReg));
        } else if (assign->value->astType == hadron::ast::ASTType::kCalculate) {
            const auto calc = reinterpret_cast<const hadron::ast::CalculateAST*>(assign->value.get());
            // Assumption is it's always a Value on the left and either a Value or Constant on the right,
            // this would happen as part of 3-address tree shaping during Syntax Analysis
            if (calc->left->astType != hadron::ast::ASTType::kValue) {
                spdlog::error("left-hand Calculate operand was not a value");
                return;
            }
            const auto left = reinterpret_cast<const hadron::ast::ValueAST*>(calc->left.get());
            int leftReg = allocator->map.find(left->nameHash)->second;
            if (calc->selector == hadron::kAddHash) {
                if (calc->right->astType == hadron::ast::ASTType::kConstant) {
                    const auto right = reinterpret_cast<const hadron::ast::ConstantAST*>(calc->right.get());
                    // addi %targetReg, %leftReg, right
                    _jit_new_node_www(jit->jitState(), jit_code_addi, JIT_V(targetReg), JIT_V(leftReg),
                        right->value.asInteger());
                } else {
                    const auto right = reinterpret_cast<const hadron::ast::ValueAST*>(calc->right.get());
                    int rightReg = allocator->map.find(right->nameHash)->second;
                    // addr %targetReg, %leftReg, %rightReg
                    _jit_new_node_www(jit->jitState(), jit_code_addr, JIT_V(targetReg), JIT_V(leftReg),
                        JIT_V(rightReg));
                }
            } else {
                spdlog::error("unsupported Calculate operation on JIT");
            }
        } else if (assign->value->astType == hadron::ast::ASTType::kConstant) {
            const auto constant = reinterpret_cast<const hadron::ast::ConstantAST*>(assign->value.get());
            // movi %targetReg, constant
            _jit_new_node_ww(jit->jitState(), jit_code_movi, JIT_V(targetReg), constant->value.asInteger());
        } else {
            spdlog::error("unsupported AST on JITing assign node");
        }
    } break;

    case hadron::ast::ASTType::kWhile: {
        const auto whileAST = reinterpret_cast<const hadron::ast::WhileAST*>(ast);
        // Assumption is condition is a calculate node
        if (whileAST->condition->astType != hadron::ast::ASTType::kInlineBlock) {
            spdlog::error("bad AST on JITting while condition node");
            return;
        }
        auto inlineBlock = reinterpret_cast<const hadron::ast::InlineBlockAST*>(whileAST->condition.get());
        if (inlineBlock->statements.size() != 1 ||
                inlineBlock->statements[0]->astType != hadron::ast::ASTType::kCalculate) {
            spdlog::error("bad AST on JITting while condition calculate node");
            return;
        }
        // conditionCheckLabel:
        jit_node_t* conditionCheckLabel = _jit_label(jit->jitState());
        jit_node_t* branch = nullptr;
        const auto calc = reinterpret_cast<const hadron::ast::CalculateAST*>(inlineBlock->statements[0].get());
        if (calc->left->astType != hadron::ast::ASTType::kValue) {
            spdlog::error("while left-hand Calculate operand was not a value!");
        }
        const auto left = reinterpret_cast<const hadron::ast::ValueAST*>(calc->left.get());
        int leftReg = allocator->map.find(left->nameHash)->second;
        if (calc->selector == hadron::kLessThanHash) {
            if (calc->right->astType == hadron::ast::ASTType::kConstant) {
                const auto right = reinterpret_cast<const hadron::ast::ConstantAST*>(calc->right.get());
                // bgei conditionFalseLabel, %leftReg, right
                branch = _jit_new_node_pww(jit->jitState(), jit_code_bgei, nullptr, JIT_V(leftReg),
                    right->value.asInteger());
            } else {
                spdlog::error("while right-hand Calculate operand was not a constant");
            }
        }

        if (whileAST->action->astType == hadron::ast::ASTType::kInlineBlock) {
            const auto inlineBlock = reinterpret_cast<const hadron::ast::InlineBlockAST*>(whileAST->action.get());
            for (const auto& expr : inlineBlock->statements) {
                jitAST(expr.get(), jit, allocator);
            }
        } else {
            spdlog::error("while action not inline block");
        }

        // jumpi conditionCheckLabel
        jit_node_t* jmpi = _jit_new_node_p(jit->jitState(), jit_code_jmpi, nullptr);
        _jit_patch_at(jit->jitState(), jmpi, conditionCheckLabel);
        _jit_patch(jit->jitState(), branch);
    } break;

    case hadron::ast::ASTType::kResult: {
        const auto result = reinterpret_cast<const hadron::ast::ResultAST*>(ast);
        if (result->value->astType == hadron::ast::ASTType::kValue) {
            const auto value = reinterpret_cast<const hadron::ast::ValueAST*>(result->value.get());
            int reg = allocator->map.find(value->nameHash)->second;
            // movr %r0, %valueReg
            _jit_new_node_ww(jit->jitState(), jit_code_movr, JIT_R0, JIT_V(reg));
        } else {
            spdlog::error("result value not value.");
        }
    } break;

    default:
        break;
    }
}

void jitBlockInline(const hadron::ast::BlockAST* block, hadron::LightningJITBlock* jit,
    hadron::RegisterAllocator* allocator) {
    // Add the variables to the register map.
    for (auto pair : block->variables) {
        allocator->map.emplace(std::make_pair(pair.first, allocator->map.size()));
    }

    for (const auto& expr : block->statements) {
        jitAST(expr.get(), jit, allocator);
    }
}
} // namespace

namespace hadron {

LightningJIT::LightningJIT() { }

LightningJIT::~LightningJIT() { }

// static
void LightningJIT::initJITGlobals() {
    init_jit(nullptr);
}

// static
void LightningJIT::finishJITGlobals() {
    finish_jit();
}

std::unique_ptr<JITBlock> LightningJIT::jitBlock(const ast::BlockAST* block) {
    auto jit = std::make_unique<LightningJITBlock>();
    _jit_prolog(jit->jitState());
    RegisterAllocator reg;
    jitBlockInline(block, jit.get(), &reg);
    // Result is presumed to be stored in R0
    _jit_retr(jit->jitState(), JIT_R0);
    _jit_epilog(jit->jitState());
    jit->emit();
    return jit;
}

} // namespace hadron