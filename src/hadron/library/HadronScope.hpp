#ifndef SRC_HADRON_LIBRARY_HADRON_SCOPE_HPP_
#define SRC_HADRON_LIBRARY_HADRON_SCOPE_HPP_

#include "hadron/library/HadronBlock.hpp"
#include "hadron/library/HadronFrame.hpp"
#include "hadron/library/Integer.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/schema/HLang/HadronScopeSchema.hpp"

namespace hadron {
namespace library {

class Scope : public Object<Scope, schema::HadronScopeSchema> {
public:
    Scope(): Object<Scope, schema::HadronScopeSchema>() {}
    explicit Scope(schema::HadronScopeSchema* instance): Object<Scope, schema::HadronScopeSchema>(instance) {}
    explicit Scope(Slot instance): Object<Scope, schema::HadronScopeSchema>(instance) {}
    ~Scope() {}

    Frame frame() const { return Frame(m_instance->frame); }
    void setFrame(Frame f) { m_instance->frame = f.slot(); }

    Scope parent() const { return Scope(m_instance->parent); }
    void setParent(Scope p) { m_instance->parent = p.slot(); }

    TypedArray<Block> blocks() const { return TypedArray<Block>(m_instance->blocks); }
    void setBlocks(TypedArray<Block> a) { m_instance->blocks = a.slot(); }

    TypedArray<Scope> subScopes() const { return TypedArray<Scope>(m_instance->subScopes); }
    void setSubScopes(TypedArray<Scope> a) { m_instance->subScopes = a.slot(); }

    int32_t frameIndex() const { return m_instance->frameIndex.getInt32(); }
    void setFrameIndex(int32_t i) { m_instance->frameIndex = Slot::makeInt32(i); }

    TypedIdentDict<Symbol, Integer> valueIndices() const {
        return TypedIdentDict<Symbol, Integer>(m_instance->valueIndices);
    }
    void setValueIndices(TypedIdentDict<Symbol, Integer> tid) {
        m_instance->valueIndices = tid.slot();
    }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_SCOPE_HPP_