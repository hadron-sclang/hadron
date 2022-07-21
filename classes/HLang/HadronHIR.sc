HadronHIR {
	var <>id;
	var <>typeFlags;
	var <>reads;
	var <>consumers;
	var <>owningBlock;

	// Generic asDotString just provides the name of the HIR.
	asDotString { |dotString, subFrames, prefix|
		var nodeName = this.class.name.asString;
		nodeName = nodeName["Hadron".size ..];
		dotString = dotString ++
		"      <tr><td>%</td></tr>\n".format(nodeName);
		^[dotString, subFrames];
	}
}

HadronBlockLiteralHIR : HadronHIR {
	var <>frame;
	var <>functionDef;

	/*
	asDotString { |dotString, subFrames, prefix|
		var subPrefix = prefix ++ id.asString;
		subFrames = subFrames.add([frame, subPrefix]);
		dotString = dotString ++
		"      <tr><td port=\"port_%\">% &#8592; BlockLiteral</td></tr>\n"
		.format(subPrefix, id);

		^[dotString, subFrames];
	}
	*/
}

HadronBranchHIR : HadronHIR {
	var <>blockId;
}

HadronBranchIfTrueHIR : HadronHIR {
	var <>condition;
	var <>blockId;
}

HadronConstantHIR : HadronHIR {
	var <>constant;
}

HadronLoadOuterFrameHIR : HadronHIR {
	var <>innerContext;
}

HadronMessageHIR : HadronHIR {
	var <>selector;
	var <>arguments;
	var <>keywordArguments;
}

HadronMethodReturnHIR : HadronHIR { }

HadronPhiHIR : HadronHIR {
	var <>name;
	var <>inputs;
	var <>isSelfReferential;
}

HadronReadFromClassHIR : HadronHIR {
	var <>classVariableArray;
	var <>arrayIndex;
	var <>valueName;
}

HadronReadFromContextHIR : HadronHIR {
	var <>offset;
	var <>valueName;
}

HadronReadFromFrameHIR : HadronHIR {
	var <>frameIndex;
	var <>frameId;
	var <>valueName;
}

HadronReadFromThisHIR : HadronHIR {
	var <>thisId;
	var <>index;
	var <>valueName;
}

HadronRouteToSuperclassHIR : HadronHIR {
	var <>thisId;
}

HadronStoreReturnHIR : HadronHIR {
	var <>returnValue;
}

HadronWriteToClassHIR : HadronHIR {
	var <>classVariableArray;
	var <>arrayIndex;
	var <>valueName;
	var <>toWrite;
}

HadronWriteToFrameHIR : HadronHIR {
	var <>frameIndex;
	var <>frameId;
	var <>valueName;
	var <>toWrite;
}

HadronWriteToThisHIR : HadronHIR {
	var <>thisId;
	var <>index;
	var <>valueName;
	var <>toWrite;
}
