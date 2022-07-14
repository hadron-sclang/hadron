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

	// buildArtifacts is an Array of HadronBuildArtifacts,
	// outputDir is a directory to build index.html and related files,
	// stopAfter is one of the stageNames after which we won't generate a report.
	*buildVisualization { |buildArtifacts, outputDir, stopAfter = \machineCode|
		var indexFile;

		File.mkdir(outputDir);
		indexFile = File.new(outputDir +/+ "index.html", "w");

		indexFile.write(
			"<html>\n"
			"  <head><title>hadron vis report</title></head>\n"
			"  <style>\n"
			"  body {{ font-family: Verdana }}\n"
			"  td.used {{ background-color: grey; color: white }}\n"
			"  td.live {{ background-color: lightGrey; color: black }}\n"
			"  </style>\n"
			"  <body>\n"
			"    <h2>hadron vis report for</h2>\n");

		buildArtifacts.do({ |artifact|
			if (artifact.methodName.notNil, {
				indexFile.write("  <h2>%:%</h2>\n".format(artifact.className, artifact.methodName));
			}, {
				indexFile.write("  <h2>%</h2>\n".format(artifact.className));
			});
		});

		indexFile.write(
			"  </body>\n"
			"</html>\n");
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

	*idString { |obj|
		var hash = obj.identityHash;
		var str;
		if (hash < 0, {
			str = "N" ++ ((-1 * hash).asString);
		}, {
			str = "P" ++ (hash.asString);
		});
		^str;
	}
}