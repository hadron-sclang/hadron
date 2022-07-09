HadronLexToken {
	var <>name; // A Symbol identifying the kind of token this is
	var <>value; // For literals, the actual computed value of the literal
	var <>lineNumber; // 1-based line number
	var <>characterNumber; // 1-based character number on that line
	var <>offset; // 0-based character offset in input string
	var <>length; // length of substring in input string
	var <>snippet; // The actual excerpted code as a Symbol
	var <>hasEscapeCharacters; // For string and symbol tokens, a boolean indicating if the token has escape characters.
}

HadronParseNode {
	var <>token;
	var <>next;
	var <>tail;

	// Returns a String with a complete dot graph description of a parse tree rooted at this node.
	asDotGraph {
		var dotString = "digraph HadronParseTree {\n  graph [fontname=helvetica];\n  node [fontname=helvetica];\n\n";
		dotString = this.prAsDotNode(dotString);
		dotString = dotString ++ "}\n";
		^dotString;
	}

	prAsDotNode { |dotString|
		var children, nodeName, nodeSerial;
		// map of names to objects.
		children = IdentityDictionary.new;
		nodeSerial = this.identityHash.abs.asString;

		// Remove "Hadron" from the start of the node class name.
		nodeName = this.class.name.asString;
		nodeName = nodeName["Hadron".size ..];
		nodeName = nodeName[.. nodeName.find("Node") - 1];

		dotString = dotString ++
		"  node_% [shape=plain label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n"
		"    <tr><td bgcolor=\"lightGray\"><b>%</b></td></tr>\n"
		"    <tr><td><font face=\"monospace\">%</font></td></tr>\n".format(nodeSerial, nodeName,
			HadronVisualizer.htmlEscapeString(token.snippet));

		if (next.notNil, {
			dotString = dotString ++ "    <tr><td port=\"next\">next</td></tr>\n";
		});

		this.class.instVarNames.do({ |name|
			switch(name,
				\token, {},
				\next, {},
				\tail, {},
				/* default */ {
					var value = this.perform(name);
					if (value.class.findRespondingMethodFor('prAsDotNode').notNil, {
						dotString = dotString ++ "    <tr><td port=\"%\">%</td></tr>\n".format(name, name);
						children.put(name, value);
					});
			});
		});
		dotString = dotString ++ "    </table>>]\n";

		if (next.notNil, {
			dotString = next.prAsDotNode(dotString);
			dotString = dotString ++ "    node_%:next -> node_%\n".format(nodeSerial, next.identityHash.abs.asString);
		});

		children.keysValuesDo({ |name, node|
			dotString = node.prAsDotNode(dotString);
			dotString = dotString ++ "    node_%:% -> node_%\n".format(nodeSerial, name, node.identityHash.abs.asString);
		});

		^dotString;
	}
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
	var <>adverb; // it is possible in some cases to add a third argument
}

// { |arguments| var variables; body }
HadronBlockNode : HadronParseNode {
	var <>arguments;
	var <>variables;
	var <>body;
}

// "target.selector(arguments, keywordArguments)"
HadronCallNode : HadronParseNode {
	var <>target;
	var <>arguments;
	var <>keywordArguments;
}

// "token[optionalNameToken] : superClassNameToken { var variables; methods }
HadronClassNode : HadronParseNode {
	var <>superclassNameToken; // can be nil
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
HadronNewNode : HadronCallNode { }

// "start, step .. stop"
HadronNumericSeriesNode : HadronParseNode {
	var <>start;
	var <>step;
	var <>stop;
}

HadronPerformListNode : HadronCallNode { }

HadronPutSeriesNode : HadronParseNode {
	var <>target;
	var <>first;
	var <>second;
	var <>last;
	var <>value;
}

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

// Implied call to .value()
HadronValueNode : HadronCallNode { }

// "token = initialValue"
HadronVarDefNode : HadronParseNode {
	var <>initialValue; // can be nil
	var <>hasReadAccessor;
	var <>hasWriteAccessor;
}

// token can be "var", "classvar", or "const"
HadronVarListNode : HadronParseNode {
	var <>definitions; // 1 or more HadronVarDefNodes
}

HadronWhileNode : HadronParseNode {
	var <>conditionBlock;
	var <>actionBlock; // can be nil
}

// See the grammar described at https://doc.sccode.org/Guides/ListComprehensions.html#Grammar
HadronListComprehensionNode : HadronParseNode {
	var <>body; // An HadronExprSeqNode
	var <>qualifiers; // One or more Hadron*QualifierNode
}

HadronGeneratorQualifierNode : HadronParseNode {
	var <>name;
	var <>exprSeq;
}

HadronGuardQualifierNode : HadronParseNode {
	var <>exprSeq;
}

HadronBindingQualifierNode : HadronParseNode {
	var <>name;
	var <>exprSeq;
}

HadronSideEffectQualifierNode : HadronParseNode {
	var <>exprSeq;
}

HadronTerminationQualifierNode : HadronParseNode {
	var <>exprSeq;
}
