HadronCFGScope {
	// The HadronCFGFrame that contains this scope.
	var <>frame;
	// Parent scope of this scope, or nil if this is the root scope.
	var <>parent;
	// An Array of HadronCFGBlock instances contained by this scope.
	var <>blocks;
	// An Array of HadronCFGScope instances contained by this scope.
	var <>subScopes;
	// The index in the HadronCFGFrame prototypeFrame array of the first local variable defined in this scope.
	var <>frameIndex;
	// An IdentityDictionary of names to index for quick membership lookup.
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
	// The HadronCFGScope that owns this block.
	var <>scope;
	// The top-level HadronCFGFrame.
	var <>frame;
	var <>id;
	// Blocks that may branch to this block.
	var <>predecessors;
	// Blocks that this block may branch to.
	var <>successors;
	var <>phis;
	var <>statements;
	// Branch and return statements executed after any HIR in statements.
	var <>exitStatements;
	// A boolean which is true if this Block has an explicit return statement in it.
	var <>hasMethodReturn;
	// The last HIR id computed by this Block.
	var <>finalValue;
	// To avoid creation of duplicate contants we track all constant values in an IdentityDictionary
	// that maps constant keys -> HIR id values.
	var <>constantValues;
	// Because you can't put nil in a Dictionary as a key, but nil is a commonly used constant, we
	// track it separately here. Either nil or an HIR id.
	var <>nilConstantValue;
	// An IdentitySet of the ConstantHIR ids, for quick membership queries.
	var <>constantIds;
}