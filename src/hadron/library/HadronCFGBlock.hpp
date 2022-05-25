#ifndef SRC_HADRON_LIBRARY_HADRON_CFGBLOCK_HPP_
#define SRC_HADRON_LIBRARY_HADRON_CFGBLOCK_HPP_

#include "hadron/library/Dictionary.hpp"
#include "hadron/library/HadronCFGFrame.hpp"
#include "hadron/library/HadronCFGScope.hpp"
#include "hadron/library/HadronHIR.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/schema/HLang/HadronCFGBlockSchema.hpp"

namespace hadron {
namespace library {

class CFGBlock : public Object<CFGBlock, schema::HadronCFGBlockSchema> {
public:
    CFGBlock(): Object<CFGBlock, schema::HadronCFGBlockSchema>() {}
    explicit CFGBlock(schema::HadronCFGBlockSchema* instance):
            Object<CFGBlock, schema::HadronCFGBlockSchema>(instance) {}
    explicit CFGBlock(Slot instance): Object<CFGBlock, schema::HadronCFGBlockSchema>(instance) {}
    ~CFGBlock() {}

    CFGScope scope() const { return CFGScope(m_instance->scope); }
    void setScope(CFGScope s) { m_instance->scope = s.slot(); }

    CFGFrame frame() const { return CFGFrame(m_instance->frame); }
    void setFrame(CFGFrame f) { m_instance->frame = f.slot(); }

    int32_t id() const { return m_instance->id.getInt32(); }
    void setId(int32_t i) { m_instance->id = Slot::makeInt32(i); }

    TypedArray<CFGBlock> predecessors() const { return TypedArray<CFGBlock>(m_instance->predecessors); }
    void setPredecessors(TypedArray<CFGBlock> a) { m_instance->predecessors = a.slot(); }

    TypedArray<CFGBlock> successors() const { return TypedArray<CFGBlock>(m_instance->successors); }
    void setSuccessors(TypedArray<CFGBlock> a) { m_instance->successors = a.slot(); }

    TypedIdentDict<Slot, Integer> constantValues() const {
        return TypedIdentDict<Slot, Integer>(m_instance->constantValues);
    }
    void setConstantValues(TypedIdentDict<Slot, Integer> tid) {
        m_instance->constantValues = tid.slot();
    }

    TypedIdentSet<Integer> constantIds() const { return TypedIdentSet<Integer>(m_instance->constantIds); }
    void setConstantIds(TypedIdentSet<Integer> tis) { m_instance->constantIds = tis.slot(); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_BLOCK_HPP_