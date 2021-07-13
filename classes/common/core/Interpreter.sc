Interpreter {
    compile { |string = ""|
        _CompileExpression
        ^nil
    }
}
