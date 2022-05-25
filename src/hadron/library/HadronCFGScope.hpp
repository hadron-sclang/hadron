#ifndef SRC_HADRON_LIBRARY_HADRON_CFGSCOPE_HPP_
#define SRC_HADRON_LIBRARY_HADRON_CFGSCOPE_HPP_

#include "hadron/library/HadronCFGBlock.hpp"
#include "hadron/library/HadronCFGFrame.hpp"
#include "hadron/library/Integer.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/schema/HLang/HadronCFGScopeSchema.hpp"

namespace hadron {
namespace library {

/*
      /-----------------\
      |                 |
      v                 |
 +----------+      +----------+
 | CFGScope |----->| CFGFrame |
 +----------+      +----------+
      |  ^          ^  |  ^
      |  |          |  |  |
      |  | /--------/  |  |
      v  | |           v  |
 +----------+      +-------+
 | CFGBlock |----->| HIR   |
 +----------+      +-------+
      ^               |
      |               |
      \---------------/
 */

class CFGScope : public Object<CFGScope, schema::HadronCFGScopeSchema> {
public:
    CFGScope(): Object<CFGScope, schema::HadronCFGScopeSchema>() {}
    explicit CFGScope(schema::HadronCFGScopeSchema* instance):
            Object<CFGScope, schema::HadronCFGScopeSchema>(instance) {}
    explicit CFGScope(Slot instance): Object<CFGScope, schema::HadronCFGScopeSchema>(instance) {}
    ~CFGScope() {}

    static CFGScope makeRootCFGScope(ThreadContext* context, CFGFrame owningFrame) {
        auto scope = CFGScope::alloc(context);
        scope.initToNil();
        scope.setFrame(owningFrame);
        scope.setFrameIndex(0);
        return scope;
    }

    static CFGScope makeSubCFGScope(ThreadContext* context, CFGScope parentCFGScope) {
        auto scope = CFGScope::alloc(context);
        scope.initToNil();
        scope.setFrame(parentCFGScope.frame());
        scope.setParent(parentCFGScope);
        scope.setFrameIndex(0);
        return scope;
    }

    CFGFrame frame() const { return CFGFrame(m_instance->frame); }
    void setFrame(CFGFrame f) { m_instance->frame = f.slot(); }

    CFGScope parent() const { return CFGScope(m_instance->parent); }
    void setParent(CFGScope p) { m_instance->parent = p.slot(); }

/*
    TypedArray<Block> blocks() const { return TypedArray<Block>(m_instance->blocks); }
    void setBlocks(TypedArray<Block> a) { m_instance->blocks = a.slot(); }
*/
    Array blocks() const { return Array(m_instance->blocks); }
    void setBlocks(Array a) { m_instance->blocks = a.slot(); }

    TypedArray<CFGScope> subScopes() const { return TypedArray<CFGScope>(m_instance->subScopes); }
    void setSubScopes(TypedArray<CFGScope> a) { m_instance->subScopes = a.slot(); }

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

#endif // SRC_HADRON_LIBRARY_HADRON_CFGSCOPE_HPP_