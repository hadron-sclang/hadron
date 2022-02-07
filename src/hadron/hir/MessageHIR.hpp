#ifndef SRC_HADRON_HIR_MESSAGE_HIR_HPP_
#define SRC_HADRON_HIR_MESSAGE_HIR_HPP_

#include "hadron/hir/HIR.hpp"

#include <vector>

namespace hadron {
namespace hir {

struct MessageHIR : public HIR {
    MessageHIR();
    virtual ~MessageHIR() = default;

    NVID selector;
    library::Symbol selectorName;
    std::vector<NVID> arguments;
    std::vector<NVID> keywordArguments;

    NVID proposeValue(NVID id) override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_MESSAGE_HIR_HPP_