// The AST represents a node in the Hadron Abstract Syntax Tree
HadronAST {
	asDotGraph {
		var dotString = "digraph HadronAbstractSyntaxTree {\n  graph [fontname=helvetica];\n  node [fontname=helvetica];\n\n";
		dotString = this.prAsDotNode(dotString);
		dotString = dotString ++ "}\n";
		^dotString;
	}

	prAsDotNode { |dotString|
		var nodeName;

		nodeName = this.class.name.asString;
		nodeName = nodeName["Hadron".size ..];
		nodeName = nodeName[.. nodeName.find("AST") - 1];
	}
}

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

	prAsDotNode { |dotString|
		if (sequence.size() > 0) {

		} {
			dotString = dotString ++
			"  ast_% [shape=plain label=<<table border="0" cellborder="1" cellspacing="0">\n"
			"    <tr><td bgcolor="lightGray"><b>%</b></td></tr></table>>]\n";
		}

		^dotString;
	}
}

HadronWhileAST : HadronAST {
	var <>conditionBlock;
	var <>repeatBlock;
}
