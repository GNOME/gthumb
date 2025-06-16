public class Gth.TreeIterator<T> {
	public TreeIterator (Gtk.TreeListModel _model) {
		model = _model;
		idx = -1;
		item = null;
	}

	public unowned T get () {
		return item;
	}

	public bool next () {
		if ((idx >= 0) && (item == null))
			return false;
		idx++;
		item = model.get_item (idx);
		return item != null;
	}

	public bool has_next () {
		return model.get_item (idx + 1) != null;
	}

	public int index () {
		return idx;
	}

	public int find_first (MatchGenericItemFunc<T> match_func) {
		while (next ()) {
			if (match_func (item)) {
				return index ();
			}
		}
		return -1;
	}

	Gtk.TreeListModel model;
	T item;
	int idx;
}
