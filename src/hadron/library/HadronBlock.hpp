#ifndef SRC_HADRON_LIBRARY_HADRON_BLOCK_HPP_
#define SRC_HADRON_LIBRARY_HADRON_BLOCK_HPP_

#include "hadron/library/Dictionary.hpp"
#include "hadron/library/HadronFrame.hpp"
#include "hadron/library/HadronHIR.hpp"
#include "hadron/library/HadronScope.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/schema/HLang/HadronBlockSchema.hpp"

namespace hadron {
namespace library {

class Block : public Object<Block, schema::HadronBlockSchema> {
public:
    Block(): Object<Block, schema::HadronBlockSchema>() {}
    explicit Block(schema::HadronBlockSchema* instance): Object<Block, schema::HadronBlockSchema>(instance) {}
    explicit Block(Slot instance): Object<Block, schema::HadronBlockSchema>(instance) {}
    ~Block() {}

    Scope scope() const { return Scope(m_instance->scope); }
    void setScope(Scope s) { m_instance->scope = s.slot(); }

    Frame frame() const { return Frame(m_instance->frame); }
    void setFrame(Frame f) { m_instance->frame = f.slot(); }

    int32_t id() const { return m_instance->id.getInt32(); }
    void setId(int32_t i) { m_instance->id = Slot::makeInt32(i); }

    TypedArray<Block> predecessors() const { return TypedArray<Block>(m_instance->predecessors); }
    void setPredecessors(TypedArray<Block> a) { m_instance->predecessors = a.slot(); }

    TypedArray<Block> successors() const { return TypedArray<Block>(m_instance->successors); }
    void setSuccessors(TypedArray<Block> a) { m_instance->successors = a.slot(); }

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