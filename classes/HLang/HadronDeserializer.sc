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
					'Array', {
						decode = Array.new;
						obj.at("_elements").do({ |item|
							var decodedItem = HadronDeserializer.prBuildInstance(item, references);
							if (decodedItem.isNil, { Error.new("Adding nil to array."); });
							decode = decode.add(decodedItem);
						});
					},
					'Char', {
						// Chars get special encoding to differentiate them from unit-length strings.
						var val = obj.at("value");
						decode = val[0];
					},
					'Class', {
						// The sclang interpreter gets really cranky trying to create new Class objects
						// or modify existing ones.
						decode = obj.at("name").asSymbol.asClass;
					},
					'IdentityDictionary', {
						// Expecting obj to be an array of dictionaries each with a key/value pair.
						decode = IdentityDictionary.new;
						obj.at("_elements").do({ |keyValue|
							var decodedKey, decodedValue;
							decodedKey = HadronDeserializer.prBuildInstance(keyValue.at("_key"), references);
							decodedValue = HadronDeserializer.prBuildInstance(keyValue.at("_value"), references);
							if (decodedKey.notNil and: { decodedValue.notNil}, {
								decode.put(decodedKey, decodedValue);
							}, {
								Error.new("failed to decode keyValue: %".format(keyValue));
							});
						});
					},
					'IdentitySet', {
						// IdentitySets prefer to have their elements inserted through the provided API,
						// rather than deserializing the underlying array directly.
						decode = IdentitySet.new;
						obj.at("_elements").do({ |item|
							var decodedItem = HadronDeserializer.prBuildInstance(item, references);
							if (decodedItem.notNil, {
								decode.add(decodedItem);
							}, {
								Error.new("failed to decode: %".format(item));
							});
						});
					},
					'Float', {
						// 'nan' and 'inf' don't encode into JSON directly
						var val = obj.at("value").asSymbol;
						if (val === 'nan', { decode = sqrt(-1.0); }, {
							if (val === 'inf', { decode = inf; }, {
								Error.new("Got unexpected value % for Float.".format(val));
							});
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
						var decodeClass = className.asClass;
						decode = decodeClass.new;
						obj.keysValuesDo({ |key, value|
							if (key[0] != $_ and: { value.notNil }, {
								var decodedValue = HadronDeserializer.prBuildInstance(value, references);
								decode.perform((key ++ "_").asSymbol, decodedValue);
							});
						});
						/*
						decodeClass.instVarNames.do({ |name|
							decode.perform((name.asString ++ "_").asSymbol,
								HadronDeserializer.prBuildInstance(obj.at(name.asString), references));
						});
						*/
				});

				reference = obj.at("_identityHash");
				if (reference.notNil, {
					references.put(reference.asSymbol, decode);
				});
			}, {
				decode = references.at(refId.asSymbol);
				if (decode.isNil, {
					Error.new("missed reference %".format(refId));
				});
			});
		}, {
			// Not a Dictionary, can use directly.
			decode = obj;
		});

		if (decode.isNil, {
			Error.new("Decoded nil");
		});

		^decode;
	}
}