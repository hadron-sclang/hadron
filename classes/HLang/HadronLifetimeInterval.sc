HadronLiveRange {
	var <>from;
	var <>to;
}

HadronLifetimeInterval {
	var <>ranges; // Ordered Array of Live Ranges
	var <>usages; // Identity set of Integers with actual usages
	var <>valueNumber;
	var <>registerNumber;
	var <>isSplit;
	var <>isSpill;
	var <>spillSlot;
}
