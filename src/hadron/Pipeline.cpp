#include "hadron/Pipeline.hpp"

#include "hadron/Block.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
//#include "hadron/Emitter.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
//#include "hadron/Hash.hpp"
//#include "hadron/Heap.hpp"
//#include "hadron/Keywords.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
//#include "hadron/LighteningJIT.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/Parser.hpp"
#include "hadron/RegisterAllocator.hpp"
#include "hadron/Resolver.hpp"
//#include "hadron/SourceFile.hpp"

namespace hadron {

Pipeline::Pipeline(): m_errorReporter(std::make_shared<ErrorReporter>()) { }

Pipeline::Pipeline(std::shared_ptr<ErrorReporter> errorReporter): m_errorReporter(errorReporter) { }

Pipeline::~Pipeline() {}

bool Pipeline::compileBlock(std::string_view code) {
    m_errorReporter->setCode(code.data());

    return true;
}

} // namespace hadron