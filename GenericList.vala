public class Gth.GenericList<T> {
	public ListStore model;

	public GenericList () {
		model = new ListStore (typeof (T));
	}

	public Iterator<T> iterator () {
		return new Iterator<T> (this);
	}

	public class Iterator<T> {
		public Iterator (GenericList<T> _list) {
			list = _list;
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
			item = list.model.get_item (idx);
			return item != null;
		}

		public bool has_next () {
			return list.model.get_item (idx + 1) != null;
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

		GenericList<T> list;
		T item;
		int idx;
	}
}

public delegate bool MatchGenericItemFunc<T> (T item);
