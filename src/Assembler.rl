%%{
    machine assembler;

    action startArgs {
        argStart = p;
        argIndex = 0;
    }
    action argEntry {
        argStart = p;
    }
    action labelArg {
        // Skip over 'label_' part of label name.
        arg[argIndex] = strtol(argStart + 6, nullptr, 10);
        ++argIndex;
    }
    action regArg {
        // Skip over '%vr' part of register name.
        arg[argIndex] = strtol(argStart + 3, nullptr, 10);
        ++argIndex;
    }
    action intArg {
        arg[argIndex] = strtol(argStart, nullptr, 10);
        ++argIndex;
    }
    action hexArg {
        // Skip over the '0x' part of integer.
        arg[argIndex] = strtol(argStart + 2, nullptr, 16);
        ++argIndex;
    }
    action addressArg {
        // Parse 32 or 64 bit addresses, skipping over the '0x' part of the address.
        if (sizeof(void*) == 8) {
            addressArg = reinterpret_cast<void*>(strtoll(argStart + 2, nullptr, 16));
        } else {
            addressArg = reinterpret_cast<void*>(strtol(argStart + 2, nullptr, 16));
        }
    }
    action eof_ok { return true; }

    label = 'label_' digit+;
    reg = ('%vr' digit+ %regArg);
    integer = ('-'? digit+ %intArg);
    hexint = ('0x' xdigit+ %hexArg);
    address = ('0x' xdigit+ %addressArg);
    labelnum = (label %labelArg);
    whitespace = (' ' | '\t')+;
    optlabel = (label ':')? whitespace?;
    comment = ';' (extend - '\n')* ('\n' >/ eof_ok); # / # slash comment to fix the syntax highlighting
    optcomment = whitespace* comment?;
    start = (whitespace %startArgs);
    ws = (whitespace %argEntry);

    main := |*
        optlabel 'addr' start reg ws reg ws reg optcomment {
            m_jit->addr(arg[0], arg[1], arg[2]);
        };
        optlabel 'addi' start reg ws reg ws integer optcomment {
            m_jit->addi(arg[0], arg[1], arg[2]);
        };
        optlabel 'movr' start reg ws reg optcomment {
            m_jit->movr(arg[0], arg[1]);
        };
        optlabel 'movi' start reg ws integer optcomment {
            m_jit->movi(arg[0], arg[1]);
        };
        optlabel 'bgei' start reg ws integer ws labelnum optcomment {
            m_jit->bgei(arg[0], arg[1]);
        };
        optlabel 'jmpi' start labelnum optcomment {
            m_jit->jmpi();
        };
        optlabel 'ldxi' start reg ws reg ws hexint optcomment {
            m_jit->ldxi(arg[0], arg[1], arg[2]);
        };
        optlabel 'alias' start reg optcomment {
            m_jit->alias(arg[0]);
        };
        optlabel 'str' start reg ws reg optcomment {
            m_jit->str(arg[0], arg[1]);
        };
        optlabel 'sti' start address ws reg optcomment {
            m_jit->sti(addressArg, arg[0]);
        };
        optlabel 'stxi' start hexint ws reg ws reg optcomment {
            m_jit->stxi(arg[0], arg[1], arg[2]);
        };
        optlabel 'prolog' optcomment {
            m_jit->prolog();
        };
        optlabel 'arg' optcomment {
            m_jit->arg();
        };
        optlabel 'getarg' start reg ws labelnum optcomment {
            m_jit->getarg(arg[0], arg[1]);
        };
        optlabel 'allocai' start integer optcomment {
            m_jit->allocai(arg[0]);
        };
        optlabel 'ret' optcomment {
            m_jit->ret();
        };
        optlabel 'retr' start reg optcomment {
            m_jit->retr(arg[0]);
        };
        optlabel 'epilog' optcomment {
            m_jit->epilog();
        };
        optlabel 'label' optcomment {
            m_jit->label();
        };
        optlabel 'patchat' start labelnum ws labelnum optcomment {
            m_jit->patchAt(arg[0], arg[1]);
        };
        optlabel 'patch' start labelnum optcomment {
            m_jit->patch(arg[0]);
        };
        optlabel 'alias' start reg optcomment {
            m_jit->alias(arg[0]);
        };
        optlabel 'unalias' start reg optcomment {
            m_jit->unalias(arg[0]);
        };
        space { /* ignore whitespace */ };
        any {
            size_t lineNumber = m_errorReporter->getLineNumber(ts);
            m_errorReporter->addError(fmt::format("Assembler error at line {} character {}: unrecognized token '{}'",
                lineNumber, ts - m_errorReporter->getLineStart(lineNumber), std::string(ts, te - ts)));
            return false;
        };
    *|;

}%%

#include "hadron/Assembler.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/VirtualJIT.hpp"

#include "fmt/core.h"

#include <array>

namespace {
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-const-variable"
    %% write data;
#   pragma GCC diagnostic pop
}

namespace hadron {

Assembler::Assembler(std::string_view code):
    m_code(code),
    m_errorReporter(std::make_shared<ErrorReporter>(true)),
    m_jit(std::make_unique<VirtualJIT>()) {
    m_errorReporter->setCode(m_code.data());
}

Assembler::Assembler(std::string_view code, std::shared_ptr<ErrorReporter> errorReporter):
    m_code(code),
    m_errorReporter(errorReporter),
    m_jit(std::make_unique<VirtualJIT>()) {}

bool Assembler::assemble() {
    // Ragel-required state variables.
    const char* p = m_code.data();
    const char* pe = p + m_code.size();
    const char* eof = pe;
    int cs;
    int act;
    const char* ts;
    const char* te;

    // Assembler state variables used for ease of parsing arguments to opcodes.
    const char* argStart = nullptr;
    int argIndex = 0;
    std::array<int, 3> arg;
    void* addressArg = nullptr;

    %% write init;

    %% write exec;

    return true;
}


} // namespace hadron