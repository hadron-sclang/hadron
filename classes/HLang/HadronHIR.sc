HadronHIR {
	var <>id;
	var <>typeFlags;
	var <>reads;
	var <>consumers;
	var <>owningBlock;
}

HadronBlockLiteralHIR : HadronHIR {
	var <>frame;
	var <>functionDef;
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
