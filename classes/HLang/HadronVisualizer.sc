HadronVisualizer {
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