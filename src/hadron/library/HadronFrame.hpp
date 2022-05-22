#ifndef SRC_HADRON_LIBRARY_HADRON_FRAME_HPP_
#define SRC_HADRON_LIBRARY_HADRON_FRAME_HPP_

#include "hadron/library/Object.hpp"
#include "hadron/schema/HLang/HadronFrameSchema.hpp"

namespace hadron {
namespace library {

class Frame : public Object<Frame, schema::HadronFrameSchema> {
public:
    Frame(): Object<Frame, schema::HadronFrameSchema>() {}
    explicit Frame(schema::HadronFrameSchema* instance): Object<Frame, schema::HadronFrameSchema>(instance) {}
    explicit Frame(Slot instance): Object<Frame, schema::HadronFrameSchema>(instance) {}
    ~Frame() {}
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_FRAME_HPP_