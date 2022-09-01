#include "hadron/library/Interpreter.hpp"

#include "hadron/ASTBuilder.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/ClassLibrary.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/Materializer.hpp"
#include "hadron/Parser.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"

namespace hadron {
namespace library {

Function Interpreter::compile(ThreadContext* context, String code) {
    auto function = library::Function();

    Lexer lexer(code.view());
    if (!lexer.lex()) { return function; }

    Parser parser(&lexer);
    if (!parser.parse(context)) { return function; }

    ASTBuilder astBuilder;
    auto ast = astBuilder.buildBlock(context, library::BlockNode(parser.root().slot()));
    if (!ast) { return function; }

    BlockBuilder blockBuilder(context->classLibrary->functionCompileContext());
    auto frame = blockBuilder.buildMethod(context, ast, true);
    if (!frame) { return function; }

    auto bytecode = Materializer::materialize(context, frame);
    if (!bytecode) { return function; }

    auto def = library::FunctionDef::alloc(context);
    def.initToNil();
    def.setCode(bytecode);
    def.setSelectors(frame.selectors());
    def.setPrototypeFrame(frame.prototypeFrame());
    def.setArgNames(frame.argumentNames());
    def.setVarNames(frame.variableNames());

    function = library::Function::alloc(context);
    function.initToNil();
    function.setDef(def);

    return function;
}

} // namespace library
} // namespace hadron
