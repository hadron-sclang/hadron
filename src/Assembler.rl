%%{
    machine assembler;

    label = lower (alnum | '_')*;
    reg = '%vr' digit+;
    integer = digit+;
    address = '0x' hex+;

    main := |*
        # semicolon marks beginning of comment until end of line
        ';' (extend - '\n')* '\n' { /* ignore comments */ };

        'addr' reg reg reg


    *|;

}%%

#include "hadron/Assembler.hpp"

#include "hadron/ErrorReporter.hpp"

namespace {
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-const-variable"
    %% write data;
#   pragma GCC diagnostic pop
}

namespace hadron {

} // namespace hadron