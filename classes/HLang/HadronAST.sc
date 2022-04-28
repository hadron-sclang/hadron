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
	var <>arrayValue;
	var <>targetNames;
	var <>lastIsRemain;
}

HadronNameAST : HadronAST {
	var <>name;
}

HadronWhileAST : HadronAST {
	var <>conditionBlock;
	var <>repeatBlock;
}
