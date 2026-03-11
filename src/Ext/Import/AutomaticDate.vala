public enum Gth.AutomaticDate {
	TODAY,
	MODIFIED,
	ORIGINAL;

	public unowned string to_state () {
		return STATE[this];
	}

	public static AutomaticDate from_state (string? state) {
		var idx = Util.enum_index (STATE, state);
		return (AutomaticDate) idx;
	}

	const string[] STATE = {
		"today",
		"modified",
		"original",
	};
}
