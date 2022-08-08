#ifndef SRC_HADRON_LIBRARY_HADRON_BUILD_ARTIFACTS_HPP_
#define SRC_HADRON_LIBRARY_HADRON_BUILD_ARTIFACTS_HPP_

#include "hadron/library/HadronAST.hpp"
#include "hadron/library/HadronCFG.hpp"
#include "hadron/library/HadronLinearFrame.hpp"
#include "hadron/library/HadronParseNode.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/schema/HLang/HadronBuildArtifactsSchema.hpp"

namespace hadron {
namespace library {

class BuildArtifacts : public Object<BuildArtifacts, schema::HadronBuildArtifactsSchema> {
public:
    BuildArtifacts(): Object<BuildArtifacts, schema::HadronBuildArtifactsSchema>() {}
    explicit BuildArtifacts(schema::HadronBuildArtifactsSchema* instance):
            Object<BuildArtifacts, schema::HadronBuildArtifactsSchema>(instance) {}
    explicit BuildArtifacts(Slot instance): Object<BuildArtifacts, schema::HadronBuildArtifactsSchema>(instance) {}
    ~BuildArtifacts() {}

    static BuildArtifacts make(ThreadContext* context) {
        auto buildArtifacts = BuildArtifacts::alloc(context);
        buildArtifacts.initToNil();
        return buildArtifacts;
    }

    Symbol sourceFile(ThreadContext* context) const { return Symbol(context, m_instance->sourceFile); }
    void setSourceFile(Symbol s) { m_instance->sourceFile = s.slot(); }

    Symbol className(ThreadContext* context) const { return Symbol(context, m_instance->className); }
    void setClassName(Symbol c) { m_instance->className = c.slot(); }

    Symbol methodName(ThreadContext* context) const { return Symbol(context, m_instance->methodName); }
    void setMethodName(Symbol m) { m_instance->methodName = m.slot(); }

    Node parseTree() const { return Node::wrapUnsafe(m_instance->parseTree); }
    void setParseTree(Node p) { m_instance->parseTree = p.slot(); }

    BlockAST abstractSyntaxTree() const { return BlockAST(m_instance->abstractSyntaxTree); }
    void setAbstractSyntaxTree(BlockAST a) { m_instance->abstractSyntaxTree = a.slot(); }

    CFGFrame controlFlowGraph() const { return CFGFrame(m_instance->controlFlowGraph); }
    void setControlFlowGraph(CFGFrame f) { m_instance->controlFlowGraph = f.slot(); }

    LinearFrame linearFrame() const { return LinearFrame(m_instance->linearFrame); }
    void setLinearFrame(LinearFrame f) { m_instance->linearFrame = f.slot(); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_BUILD_ARTIFACTS_HPP_