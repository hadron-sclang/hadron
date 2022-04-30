// The AST represents a node in the Hadron Abstract Syntax Tree
HadronAST { }

HadronAssignAST : HadronAST {
	var <>name;
	var <>value;
}

HadronBlockAST : HadronAST {
	var <>argumentNames;
	var <>argumentDefaults;
	var <>hasVarArg;
	var <>statements;
}

HadronConstantAST : HadronAST {
	var <>constant;
}

HadronDefineAST : HadronAST {
	var <>name;
	var <>value;
}

HadronEmptyAST : HadronAST {}

HadronIfAST : HadronAST {
	var <>condition;
	var <>trueBlock;
	var <>falseBlock;
}

HadronMessageAST : HadronAST {
	var <>selector;
	var <>arguments;
	var <>keywordArguments;
}

HadronMethodReturnAST : HadronAST {
	var <>value;
}

HadronMultiAssignAST : HadronAST {
    // The value that should evaluate to an Array.
	var <>arrayValue;
    // The in-order sequence of names or anonymous targets that receive the individual elements of arrayValue.
	var <>targetNames;
    // If true, the last element receives the rest of the array, otherwise false.
	var <>lastIsRemain;
}

HadronNameAST : HadronAST {
	var <>name;
}

HadronSequenceAST : HadronAST {
	var <>sequence;
}

HadronWhileAST : HadronAST {
	var <>conditionBlock;
	var <>repeatBlock;
}
