HadronLinearFrame {
	var <>instructions;
	var <>vRegs;
	var <>blockOrder;
	var <>blockLabels;
	var <>blockRanges;
	var <>liveIns; // per-block sets of values live at entry to block
	var <>valueLifetimes;
	var <>numberOfSpillSlots;
	var <>hirToRegMap;
}
