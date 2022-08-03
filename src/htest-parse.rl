%%{
    machine parser;

    main := |*

        '// RUN: ' (any - '\n')+
    *|;
}%%