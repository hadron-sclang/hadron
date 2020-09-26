%%{
    machine lexer;

    name = '~'?[a-z][a-zA-Z0-9_]+;
    const = 'const';
    var = 'var';
    nil = 'nil';
    true = 'true';
    false = 'false';

    integer = '-'?[0-9]+;
    hexInteger = '-'?[0-9]+'x'[0-9]+;
    radixInteger = '-'?[0-9]+'r'[a-zA-Z0-9]+;
    float = '-'?[0-9]+'.'[0-9]+;
    radixFloat = '-'?[0-9]+'r'[a-zA-Z0-9]+'.'[a-zA-Z0-9]+;

    main := |*
        integer =>  { spdlog::warn("integer"); };
        hexInteger => { spdlog::warn("hexInteger"); };
        radixInteger => { spdlog::warn("radixInteger"); };
        float => { spdlog::warn("float"); };
        radixFloat => { spdlog::warn("radixFloat"); };
    *|;
}%%

#include "Lexer.hpp"

#include "spdlog/spdlog.h"

namespace {
    %% write data;
}

namespace hadron {

Lexer::Lexer(const char* code):
    p(code) {
    %% write init;
}

bool Lexer::next() {
    %% write exec;
    return true;
}

bool Lexer::isError() {
    return false;
}

bool Lexer::isEOF() {
    return false;
}

} // namespace hadron

