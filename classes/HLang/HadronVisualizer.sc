HadronVisualizer {
	classvar <stageNames;

	*initClass {
		stageNames = IdentityDictionary.newFrom(
			(
				parse: 1,
				abstractSyntaxTree: 2,
				controlFlowGraph: 3,
				linearFrame: 4,
				lifetimeAnalysis: 5,
				registerAllocation: 6,
				machineCode: 7
			)
		);
	}

	// buildArtifacts is an Array of HadronBuildArtifacts, outputDir is a directory to build index.html and related files, stopAfter is one of the stageNames
	// after which we won't generate a report.
	*buildVisualization { |buildArtifacts, outputDir, stopAfter = \machineCode|

	}

	*htmlEscapeString { |str|
		str = str.replace("\t", "&nbsp;&nbsp;&nbsp;&nbsp;");
		str = str.replace(" ", "&nbsp;");
		str = str.replace("{", "&#123;");
		str = str.replace("[", "&#91;");
		str = str.replace("<", "&lt;");
		str = str.replace(">", "&gt;");
		^str;
	}
}