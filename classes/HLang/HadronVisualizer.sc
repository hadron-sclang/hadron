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
	*buildVisualization { |buildArtifacts, outputDir, dotPath, stopAfter = \machineCode|
		var stopAfterStage, indexFile;
		stopAfterStage = stageNames.at(stopAfter);

		File.mkdir(outputDir);
		indexFile = File.new(outputDir +/+ "index.html", "w");

		indexFile.write(
			"<html>\n"
			"  <head><title>hadron vis report</title></head>\n"
			"  <style>\n"
			"    body { font-family: Verdana }\n"
			"    td.used { background-color: grey; color: white }\n"
			"    td.live { background-color: lightGrey; color: black }\n"
			"  </style>\n"
			"  <body>\n");

		buildArtifacts.do({ |artifact|
			indexFile.write("  <h2>%:%</h2>\n"
				.format(artifact.className, artifact.methodName));
			indexFile.write("  <p>%:%</p>\n"
				.format(artifact.sourceFile, artifact.parseTree.token.lineNumber));

			if (artifact.parseTree.notNil, {
				var commandLine, parseBase;
				parseBase = outputDir +/+ "parse_" ++ HadronVisualizer.idString(artifact.parseTree);
				File.use(parseBase ++ ".dot", "w", { |f| f.write(artifact.parseTree.asDotGraph); });
				commandLine = dotPath ++ " -Tsvg -o% %".format(parseBase ++ ".svg", parseBase ++ ".dot");
				commandLine.unixCmd({}, false);
				indexFile.write("  <h3>parse tree</h3>\n  <img src=\"%\"/>\n".format(parseBase ++ ".svg"));
			});

			if (stopAfterStage > stageNames.at(\parse) and: { artifact.abstractSyntaxTree.notNil }, {
				var commandLine, astBase;
				astBase = outputDir +/+ "ast_" ++ HadronVisualizer.idString(artifact.abstractSyntaxTree);
				File.use(astBase ++ ".dot", "w", { |f| f.write(artifact.abstractSyntaxTree.asDotGraph); });
				commandLine = dotPath ++ " -Tsvg -o% %".format(astBase ++ ".svg", astBase ++ ".dot");
				commandLine.unixCmd({}, false);
				indexFile.write("  <h3>abstract syntax tree</h3>\n  <img src=\"%\"/>\n".format(astBase ++ ".svg"));
			});

			if (stopAfterStage > stageNames.at(\abstractSyntaxTree) and: { artifact.controlFlowGraph.notNil }, {
				var commandLine, cfgBase;
				cfgBase = outputDir +/+ "cfg_" ++ HadronVisualizer.idString(artifact.controlFlowGraph);
				File.use(cfgBase ++ ".dot", "w", { |f| f.write(artifact.controlFlowGraph.asDotGraph); });
				commandLine = dotPath ++ " -Tsvg -o% %".format(cfgBase ++ ".svg", cfgBase ++ ".dot");
				commandLine.unixCmd({}, false);
				indexFile.write("  <h3>control flow graph</h3>\n  <img src=\"%\"/>\n".format(cfgBase ++ ".svg"));
			});
		});

		indexFile.write(
			"  </body>\n"
			"</html>\n");

		indexFile.close();
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