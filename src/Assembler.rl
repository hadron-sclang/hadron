%%{
    machine assembler;

    # sA for startArgs
    action sA {
        argStart = p;
        argIndex = 0;
    }
    # aE for argEntry
    action aE {
        argStart = p;
    }
    # lA for labelArg
    action lA {
        // skip over label_ part of label name
        args[argIndex] = strtol(argStart + 6, nullptr, 10);
        ++argIndex;
    }
    # rA for regArg
    action rA {
        // skip over %vr part of register name
        args[argIndex] = strtol(argStart + 3, nullptr, 10);
        ++argIndex;
    }
    # iA for intArg
    action iA {
        args[argIndex] = strtol(argStart, nullptr, 10);
        ++argIndex;
    }
    # hA for hexArg
    action hA {
        // skip over the 0x part of integer
        args[argIndex] = strtol(argStart + 2, nullptr, 16);
        ++argIndex;
    }
    action eof_ok { return true; }

    label = 'label_' digit+;
    reg = '%vr' digit+;
    integer = '-'? digit+;
    addr = '0x' xdigit+;
    optlabel = (label ':')?;
    comment = ';' (extend - '\n')* ('\n' >/ eof_ok); # / # slash comment to fix the syntax highlighting
    ws = (' ' | '\t')+;

    main := |*
        optlabel ws? 'addr' (ws %sA) (reg %rA) (ws %aE) (reg %rA) (ws %aE) (reg %rA) space* comment? {
            m_jit->addr(args[0], args[1], args[2]);
        };
        optlabel ws? 'addi' (ws %sA) (reg %rA) (ws %aE) (reg %rA) (ws %aE) (integer %iA) space* comment? {
            m_jit->addi(args[0], args[1], args[2]);
        };
        optlabel ws? 'movr' (ws %sA) (reg %rA) (ws %aE) (reg %rA) space* comment? {
            m_jit->movr(args[0], args[1]);
        };
        optlabel ws? 'movi' (ws %sA) (reg %rA) (ws %aE) (integer %iA) space* comment? {
            m_jit->movi(args[0], args[1]);
/*
    void sti(Address address, Reg value) override;
    void stxi(int offset, Reg address, Reg value) override;
    void prolog() override;
    Label arg() override;
    void getarg(Reg target, Label arg) override;
    void allocai(int stackSizeBytes) override;
    void ret() override;
    void retr(Reg r) override;
    void epilog() override;
    Label label() override;
    void patchAt(Label target, Label location) override;
    void patch(Label label) override;
    void alias(Reg r);
    void unalias(Reg r);
*/
        };
        optlabel ws? 'bgei' (ws %sA) (reg %rA) (ws %aE) (integer %iA) (ws %aE) (label %lA) space* comment? {
            m_jit->bgei(args[0], args[1]);
        };
        optlabel ws? 'jmpi' (ws %sA) (label %lA) space* comment? {
            m_jit->jmpi();
        };
        optlabel ws? 'ldxi' (ws %sA) (reg %rA) (ws %aE) (reg %rA) (ws %aE) (addr %hA) space* comment? {
            m_jit->ldxi(args[0], args[1], args[2]);
        };
        optlabel ws? 'alias' (ws %sA) (reg %rA) space* comment? {
            m_jit->alias(args[0]);
        };
        optlabel ws? 'str' (ws %sA) (reg %rA) (ws %aE) (reg %rA) space* comment? {
            m_jit->str(args[0], args[1]);
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
    m_errorReporter(std::make_shared<ErrorReporter>(false)),
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

    const char* argStart = nullptr;
    int argIndex = 0;
    std::array<int, 3> args;

    %% write init;

    %% write exec;

    return true;
}


} // namespace hadron