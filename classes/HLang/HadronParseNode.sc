HadronLexToken {
	var <>name; // A Symbol identifying the kind of token this is
	var <>snippet; // The code snippet represented by this token, a Symbol
	var <>value; // For literals, the actual computed value of the literal
	var <>lineNumber; // 1-based line number
	var <>characterNumber; // 1-based character number on that line
}

HadronParseNode {
	var <>token;
	var <>next;
}

HadronCallBaseNode : HadronParseNode {
	var <>target;
	var <>arguments;
	var <>keywordArguments;
}

// Alphabetical list of nodes returned by the parse tree.
HadronArgListNode : HadronParseNode {
	var <>varList;
	var <>varArgsNameToken; // can be nil (for no varargs) or a HadronLexToken
}

// "targetArray[indexArgument]"
HadronArrayReadNode : HadronParseNode {
	var <>targetArray;
	var <>indexArgument;
}

// "targetArray[indexArgument] = value"
HadronArrayWriteNode : HadronParseNode {
	var <>targetArray;
	var <>indexArgument;
	var <>value;
}

// "name = value"
HadronAssignNode : HadronParseNode {
	var <>name;
	var <>value;
}

// "leftHand token rightHand"
HadronBinopCallNode : HadronParseNode {
	var <>leftHand;
	var <>rightHand;
}

// { |arguments| var variables; body }
HadronBlockNode : HadronParseNode {
	var <>arguments;
	var <>variables;
	var <>body;
}

// "target.selector(arguments, keywordArguments)"
HadronCallNode : HadronCallBaseNode { }

// "token[optionalNameToken] : superClassNameToken { var variables; methods }
HadronClassNode : HadronParseNode {
	var <>superClassNameToken; // can be nil
	var <>optionalNameToken; // can be nil
	var <>variables;
	var <>methods;
}

// +token { methods }
HadronClassExtensionNode : HadronParseNode {
	var <>methods;
}

// A literal Array, for example "[1, 2, 3]"
HadronCollectionNode : HadronParseNode {
	var <>className; // Either a HadronNameNode or nil. If nil the default class name is Array.
	var <>elements;
}

// "target[first, second .. last]"
HadronCopySeriesNode : HadronParseNode {
	var <>target;
	var <>first;
	var <>second;
	var <>last;
}

// "_"
HadronCurryArgumentNode : HadronParseNode { }

// ""
HadronEmptyNode : HadronParseNode { }

// "~token"
HadronEnvironmentAtNode : HadronParseNode { }

// "~token = value"
HadronEnvironmentPutNode : HadronParseNode {
	var <>value;
}

// A literal Event like "(a: 1)"
HadronEventNode : HadronParseNode {
	var <>elements; // expected to be key/value pairs
}

// 0 or more expressions chained together via Node.next values
HadronExprSeqNode : HadronParseNode {
	var <>expr;
}

// "if (condition, { trueBlock }, { elseBlock });
HadronIfNode : HadronParseNode {
	var <>condition;
	var <>trueBlock;
	var <>elseBlock; // may be nil
}

// "key: value"
HadronKeyValueNode : HadronParseNode {
	var <>key;
	var <>value;
}

// "token { body }"
HadronMethodNode : HadronParseNode {
	var <>isClassMethod;
	var <>primitiveToken; // either nil or a Token
	var <>body;
}

// "#targets = value"
HadronMultiAssignNode : HadronParseNode {
	var <>targets; // A HadronMultiAssignVarsNode
	var <>value;
}

// "names .. rest"
HadronMultiAssignVarsNode : HadronParseNode {
	var <>names; // HadronNameNodes
	var <>rest; // Can be nil or a HadronNameNode
}

// "token"
HadronNameNode : HadronParseNode { }

// "token()"
HadronNewNode : HadronParseNode { }

// "start, step .. stop"
HadronNumericSeriesNode : HadronParseNode {
	var <>start;
	var <>step;
	var <>stop;
}

HadronPerformListNode : HadronParseNode { }

// "^valueExpr"
HadronReturnNode : HadronParseNode {
	var <>valueExpr;
}

// "start.series(step, last)"
HadronSeriesNode : HadronParseNode {
	var <>start;
	var <>step;
	var <>last;
}

HadronSeriesIterNode : HadronParseNode {
	var <>start;
	var <>step;
	var <>last;
}

// "target.token = value"
HadronSetterNode : HadronParseNode {
	var <>target;
	var <>value;
}

// Holds any literal that fits in a slot with the exception of Strings and Symbols
HadronSlotNode : HadronParseNode {
	var <>value;
}

// "token" - note that SC allows for String concatenation so |next| may contain additional HadronStringNodes
HadronStringNode : HadronParseNode { }

HadronSymbolNode : HadronParseNode { }

// "token = initialValue"
HadronVarDefNode : HadronParseNode {
	var <>initialValue; // can be nil
}

// token can be "var", "classvar", or "const"
HadronVarListNode : HadronParseNode {
	var <>definitions; // 1 or more HadronVarDefNodes
}
