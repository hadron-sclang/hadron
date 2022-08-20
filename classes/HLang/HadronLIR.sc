HadronLIR {
	var <>vReg;       // The virtual register number this value represents, or nil
	var <>typeFlags;  // An int32 of type flags
	var <>reads;      // an IdentitySet of other vRegs this LIR reads

	// Built during register allocation, a map of all virtual registers in |arguments| and |vReg| to physical registers.
	var <>locations;

	// Due to register allocation and SSA form deconstruction any HIR operand may have a series of moves to and from
    // physical registers and/or spill storage. Record them here for scheduling later during machine code generation.
    // The keys are origins and values are destinations. Positive integers (and 0) indicate register numbers, and
    // negative values indicate spill slot indices, with spill slot 0 reserved for register move cycles. Move scheduling
    // requires origins be copied only once, so enforcing unique keys means trying to insert a move from an origin
    // already scheduled for a move is an error. These are *predicate* moves, meaning they are executed before the HIR.
	var <>moves;
}

// vReg <- origin
HadronAssignLIR : HadronLIR {
	var <>origin;
}

HadronBranchLIR : HadronLIR {
	var <>labelId; // Label to branch to.
}

HadronBranchIfTrueLIR : HadronLIR {
	var <>condition;
	var <>labelId;
}

HadronBranchToRegisterLIR : HadronLIR {
	var <>address;
}

HadronInterruptLIR : HadronLIR {
	var <>interruptCode;
}

HadronLabelLIR : HadronLIR {
	var <>labelId;
	var <>predecessors;
	var <>successors;
	var <>phis;
	var <>loopReturnPredIndex;
}

HadronLoadConstantLIR : HadronLIR {
	var <>constant;
}

HadronLoadFromPointerLIR : HadronLIR {
	var <>pointer;
	var <>offset;
}

HadronPhiLIR : HadronLIR {
	var <>inputs;
}

HadronPopFrameLIR : HadronLIR {
}

HadronStoreToPointerLIR : HadronLIR {
	var <>pointer;
	var <>offset;
	var <>toStore;
}