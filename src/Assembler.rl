%%{
    machine assembler;

    label = lower (alnum | '_')*;


    main := |*
        # semicolon marks beginning of comment until end of line
        ';' (extend - '\n')* '\n' { /* ignore comments */ };



    *|;

}%%