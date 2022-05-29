HadronCFGScope {
	var <>frame;
	var <>parent;
	var <>blocks;
	var <>subScopes;
	var <>frameIndex;
	var <>valueIndices;
}

HadronCFGFrame {
	// Function literals can capture values from outside frames, so we include a pointer to the InlineBlockHIR in the
    // containing frame to support search of those frames for those values.
	var <>outerBlockHIR;

	// The Method object described by this Frame.
	var <>method;

    // If true, the last argument named in the list is a variable argument array.
	var <>hasVarArgs;

	// Flattened variable name array.
	var <>variableNames;

    // Initial values, for concatenation onto the prototypeFrame.
	var <>prototypeFrame;

    // Any Blocks defined in this frame that can't be inlined must be tracked in the method->selectors field, to prevent
    // their premature garbage collecti	on, and also allow runtime access. During this stage of compilation they are
    // tracked as BlockLiteralHIR instructions, which are later materialized into FunctionDef instances and appended to
    // selectors.
	var <>innerBlocks;
	var <>selectors;

	// The root variable Scope object this frame contains.
	var <>rootScope;

    // Map of value IDs as index to HIR objects. During optimization HIR can change, for example simplifying MessageHIR
    // to a constant, so we identify values by their NVID and maintain a single frame-wide map here of the authoritative
    // map between IDs and HIR.
	var <>values;

    // Counter used as a serial number to uniquely identify blocks.
	var <>numberOfBlocks;
}

HadronCFGBlock {
	var <>scope;
	var <>frame;
	var <>id;
	var <>predecessors;
	var <>successors;
	var <>phis;
	var <>statements;
	// Branch and return statements executed after any HIR in statements.
	var <>exitStatements;
	var <>hasMethodReturn;
	var <>finalValue;
	var <>constantValues;
	var <>nilConstantValue;
	var <>constantIds;
}