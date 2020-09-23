# generates Lexer.cpp

%%{
    machine lexer;

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
    m_code(code) {}

bool Lexer::next() {
    %% write init;
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

