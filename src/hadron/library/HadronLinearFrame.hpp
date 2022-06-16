#ifndef SRC_HADRON_LIBRARY_HADRON_LINEAR_FRAME_HPP_
#define SRC_HADRON_LIBRARY_HADRON_LINEAR_FRAME_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/HadronLIR.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/schema/HLang/HadronLinearFrameSchema.hpp"

namespace hadron {
namespace library {

class LinearFrame : public Object<LinearFrame, schema::HadronLinearFrameSchema> {
public:
    LinearFrame(): Object<LinearFrame, schema::HadronLinearFrameSchema>() {}
    explicit LinearFrame(schema::HadronLinearFrameSchema* instance):
            Object<LinearFrame, schema::HadronLinearFrameSchema>(instance) {}
    explicit LinearFrame(Slot instance): Object<LinearFrame, schema::HadronLinearFrameSchema>(instance) {}
    ~LinearFrame() {}

    TypedArray<LIR> instructions() const { return TypedArray<LIR>(m_instance->instructions); }
    void setInstructions(TypedArray<LIR> inst) { m_instance->instructions = inst.slot(); }

    TypedArray<LIR> vRegs() const { return TypedArray<LIR>(m_instance->vRegs); }
    void setVRegs(TypedArray<LIR> v) { m_instance->vRegs = v.slot(); }

    TypedArray<LabelId> blockOrder() const { return TypedArray<LabelId>(m_instance->blockOrder); }
    void setBlockOrder(TypedArray<LabelId> bo) { m_instance->blockOrder = bo.slot(); }

    TypedArray<LabelLIR> blockLabels() const { return TypedArray<LabelLIR>(m_instance->blockLabels); }
    void setBlockLabels(TypedArray<LabelLIR> lbls) { m_instance->blockLabels = lbls.slot(); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_LINEAR_FRAME_HPP_