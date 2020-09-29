%%{
    machine lexer;

    name = '~'? lower [a-zA-Z0-9_]+;
    const = 'const';
    var = 'var';
    nil = 'nil';
    true = 'true';
    false = 'false';

    integer = '-'? digit+;
    hexInteger = '-'? digit+ 'x' xdigit+;
    radixInteger = '-'? digit+ 'r' alnum+;
    float = '-'? digit+ '.' digit+;
    radixFloat = '-'? digit+' r' alnum+ '.' alnum+;

    main :=

}%%

#include "Lexer.hpp"

#include "spdlog/spdlog.h"

namespace {
    %% write data;
}

namespace hadron {

Lexer::Lexer(std::string_View code):
    p(code.data()),
    pe(code.data() + code.size()),
    eof(pe) {
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

