public class Gth.TreeIterator<T> {
	public TreeIterator (Gtk.TreeListModel _model) {
		model = _model;
		idx = -1;
		row = null;
	}

	public unowned Gtk.TreeListRow? get_row () {
		return row;
	}

	public T? get_data () {
		if (row == null) {
			return null;
		}
		return (T) row.item;
	}

	public bool next () {
		if ((idx >= 0) && (row == null))
			return false;
		idx++;
		row = model.get_row (idx) as Gtk.TreeListRow;
		return row != null;
	}

	public bool has_next () {
		return model.get_row (idx + 1) != null;
	}

	public int index () {
		return idx;
	}

	public int find_first (MatchGenericItemFunc<T> match_func) {
		while (next ()) {
			if (match_func (row)) {
				return index ();
			}
		}
		return -1;
	}

	Gtk.TreeListModel model;
	Gtk.TreeListRow row;
	int idx;
}
