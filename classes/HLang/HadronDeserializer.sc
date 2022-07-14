HadronDeserializer {
	*fromJSONFile { |jsonFile|
		var jsonRoot, references;
		jsonRoot = jsonFile.parseYAMLFile;
		references = IdentityDictionary.new;
		^HadronDeserializer.prBuildInstance(jsonRoot, references);
	}

	*prBuildInstance { |obj, references|
		var decode, refId;
		if (obj.class.asSymbol === 'Dictionary', {
			refId = obj.at("_reference");
			if (refId.isNil, {
				var reference;
				var className = obj.at("_className").asSymbol;
				switch (className,
					'Float', {
						// 'nan' and 'inf' don't encode into JSON directly
						var val = obj.at("value").asSymbol;
						if (val === 'nan', { decode = sqrt(-1.0); }, {
							if (val === 'inf', { decode = inf; }, {
								Error.new("Got unexpected value % for Float.".format(val));
							});
						});
					},
					'Char', {
						// Chars get special encoding to differentiate them from unit-length strings.
						var val = obj.at("value");
						decode = val[0];
					},
					'Array', {
						decode = Array.new;
						obj.at("_elements").do({ |item|
							decode = decode.add(HadronDeserializer.prBuildInstance(item, references));
						});
					},
					'SymbolArray', {
						decode = SymbolArray.new;
						obj.at("_elements").do({ |item|
							decode = decode.add(item.asSymbol);
						});
					},
					'String', {
						decode = obj.at("_elements");
					},
					/* default */ {
						var decodeClass, reference;
						decodeClass = className.asClass;
						decode = decodeClass.new;
						decodeClass.instVarNames.do({ |name|
							decode.perform((name.asString ++ "_").asSymbol,
								HadronDeserializer.prBuildInstance(obj.at(name.asString), references));
						});
				});

				reference = obj.at("_identityHash");
				if (reference.notNil, {
					references.put(reference.asInteger, decode);
				});
			}, {
				decode = references.at(refId.asInteger);
			});
		}, {
			// Not a Dictionary, can use directly.
			decode = obj;
		});

		^decode;
	}
}