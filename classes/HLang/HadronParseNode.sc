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

// A literal Array, for example "[1, 2, 3]"
HadronArrayNode : HadronParseNode {
	var <>elements;
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

HadronClassExtensionNode : HadronParseNode {
	var <>methods;
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

// A new collection. Unlike HadronArrayNode this can support a class name different than array, and
// the elements can be arbitary expressions instead of being restricted to literals.
HadronNewCollectionNode : HadronParseNode {
	var <>className; // Either a HadronNameNode or nil. If nil the default class name is Array.
	var <>elements;
}