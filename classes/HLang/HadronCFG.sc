HadronCFGFrame {
	// Function literals can capture values from outside frames,
	// so we include a pointer to the InlineBlockHIR in the
    // containing frame to support search of those frames for those values.
	var <>outerBlockHIR;

    // If true, the last argument named in the list is a variable argument array.
	var <>hasVarArgs;

	// Flattened variable name array.
	var <>variableNames;

    // Initial values, for concatenation onto the prototypeFrame.
	var <>prototypeFrame;

    // Any Blocks defined in this frame that can't be inlined must be
	// tracked in the method->selectors field, to prevent
    // their premature garbage collecti	on, and also allow runtime access.
	// During this stage of compilation they are
    // tracked as BlockLiteralHIR instructions, which are later materialized
	// into FunctionDef instances and appended to selectors.
	var <>innerBlocks;
	var <>selectors;

	// The root variable Scope object this frame contains.
	var <>rootScope;

    // Map of value IDs as index to HIR objects. During optimization HIR
	// can change, for example simplifying MessageHIR
    // to a constant, so we identify values by their NVID and maintain
	// a single frame-wide map here of the authoritative
    // map between IDs and HIR.
	var <>values;

    // Counter used as a serial number to uniquely identify blocks.
	var <>numberOfBlocks;

	// Top-level dot string function. Uses asDotVerts and asDotEdges to
	// save edges and verts respectively. Subscopes render in subgraphs,
	// but the edges don't render correctly unless we specify them in
	// the top-level graph context, which is why we render them separately.
	asDotGraph {
		var dotString = "digraph HadronControlFlow {\n"
		"  graph [fontname=helvetica];\n"
		"  node [fontname=helvetica];\n\n";

		dotString = rootScope.asDotVerts(dotString);
		dotString = rootScope.asDotEdges(dotString);
		dotString = dotString ++ "}\n";

		^dotString;
	}

}

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

	asDotVerts { |dotString, prefix = ""|
		var idString, subFrames;

		idString = HadronVisualizer.idString(this);
		dotString = dotString ++ "  subgraph cluster_%_% {\n".format(prefix, idString);
		subFrames = Array.new;
		blocks.do({ |block|
			#dotString, subFrames = block.asDotVerts(dotString, subFrames, prefix);
		});
		subScopes.do({ |scope|
			dotString = scope.asDotVerts(dotString, prefix);
		});
		subFrames.do({ |framePrefixPair|
			dotString = framePrefixPair.first.rootScope.asDotVerts(dotString, framePrefixPair.last);
		});

		dotString = dotString ++ "  }\n";

		^dotString;
	}

	asDotEdges { |dotString, prefix = ""|
/*
    for block in scope['blocks']:
        for hir in block['statements']:
            if hir['opcode'] == 'BlockLiteral':
                pfx = prefix + str(hir['value']['id'])
                dotFile.write('    block_{}_{}:port_{} -> block_{}_0 [style="dashed"]\n'.format(prefix, block['id'],
                        pfx, pfx))
                saveBlockGraph(hir['frame']['rootScope'], dotFile, pfx)
        for succ in block['successors']:
            dotFile.write('    block_{}_{} -> block_{}_{}\n'.format(prefix, block['id'], prefix, succ))
    for subScope in scope['subScopes']:
        saveBlockGraph(subScope, dotFile, prefix)
*/

		^dotString;
	}
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

	// Usually nil. If non-nil, this Block is a loop header/condition block, and the value is an Integer index
	// into the predecessor array for the repeat block that branches back to this block.
	var <>loopReturnPredIndex;

	asDotVerts { |dotString, subFrames, prefix = ""|
		dotString = dotString ++
		"    block_%_% [shape=plain label=<<table border=\"0\" cellpadding=\"6\" cellborder=\"1\" cellspacing=\"0\">\n"
		"      <tr><td bgcolor=\"lightGray\"><b>Block %</b></td></tr>\n".format(prefix, id, id);
		phis.do({ |phi|
			#dotString, subFrames = phi.asDotString(dotString, subFrames, prefix);
		});
		statements.do({ |statement|
			#dotString, subFrames = statement.asDotString(dotString, subFrames, prefix);
		});

		dotString = dotString ++ "      </table>]\n";
		^[dotString, subFrames];
	}
}