HadronDeserializer {
	*fromJSONFile { |jsonFile|
		var jsonRoot, references;
		jsonRoot = jsonFile.parseYAMLFile;
		references = IdentityDictionary.new;
		^HadronDeserializer.prBuildInstance(jsonRoot, references);
	}

	*prBuildInstance { |obj, references|
		var decode, refId;
		switch (obj.class.asSymbol,
			'Dictionary', {
				refId = obj.at("_reference");
				if (refId.isNil, {
					var className, decodeClass, reference;
					className = obj.at("_className").asSymbol;
					decodeClass = className.asClass;
					decode = decodeClass.new;
					reference = obj.at("_identityHash").asInteger;
					references.put(reference, decode);
					decodeClass.instVarNames.do({ |name|
						decode.perform((name.asString ++ "_").asSymbol,
							HadronDeserializer.prBuildInstance(obj.at(name.asString), references));
					});
				}, {
					decode = references.at(refId.asInteger);
				});
			},
			'Array', {
				decode = Array.new;
				obj.do({ |item|
					decode = decode.add(HadronDeserializer.prBuildInstance(item, references));
				});
			},
			/* default */ {
				// Not a Dictionary, can use directly.
				decode = obj;
		});

		^decode;
	}
}