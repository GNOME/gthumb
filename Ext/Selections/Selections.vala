public class Gth.Selections {
	public Selections () {
		entries = new Selection[3];
		for (var i = 0; i < Selection.MAX_SELECTIONS; i++) {
			entries[i] = new Selection (i + 1);
		}
	}

	public Selection? get_selection (uint number) {
		return ((number > 0) && (number <= Selection.MAX_SELECTIONS)) ? entries[number - 1] : null;
	}

	public Selection? get_selection_from_file (File file) {
		var number = Selection.get_selection_number (file);
		return get_selection (number);
	}

	Selection[] entries;
}
