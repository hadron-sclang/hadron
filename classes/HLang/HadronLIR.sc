HadronLIR {
	var <>vReg;  // The virtual register number this value represents, or nil
	var <>typeFlags;  // An int32 of type flags
	var <>reads;  // an IdentitySet of other vRegs this LIR reads
	var <>moves; // an IndentityDictionary of moves
}