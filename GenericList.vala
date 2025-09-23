public class Gth.GenericList<T> {
	public ListStore model;

	public GenericList () {
		model = new ListStore (typeof (T));
	}

	public ListStoreIterator<T> iterator () {
		return new ListStoreIterator<T> (model);
	}

	public T? first () {
		return model.get_item (0);
	}

	public T? get (int pos) {
		return model.get_item (pos);
	}

	public inline bool is_empty () {
		return model.n_items == 0;
	}

	public inline uint length () {
		return model.n_items;
	}

	public int find (T _item) {
		var iter = new ListIterator<T> (model);
		return iter.find_item (_item);
	}

	public void copy (GenericList<T> other) {
		model.remove_all ();
		foreach (unowned var item in other) {
			model.append ((Object) item);
		}
	}

	public GenericList<T> duplicate () {
		var other = new GenericList<T>();
		other.copy (this);
		return other;
	}

	public bool remove (T item) {
		var pos = find (item);
		if (pos < 0) {
			return false;
		}
		model.remove ((uint) pos);
		return true;
	}

	public void sort (CompareDataFunc<T> func) {
		model.sort (func);
	}
}

public class Gth.IterableList<T> {
	public ListModel model;

	public IterableList (ListModel _model) {
		model = _model;
	}

	public ListIterator<T> iterator () {
		return new ListIterator<T> (model);
	}
}

public class Gth.ListIterator<T> {
	public ListIterator (ListModel _model) {
		model = _model;
		idx = -1;
		item = null;
		started = false;
		deleted = false;
	}

	public unowned T get () {
		return item;
	}

	public bool previous () {
		if (started && (item == null)) {
			return false;
		}
		if (!started) {
			idx = (int) model.get_n_items () - 1;
			started = true;
		}
		else {
			idx = idx - 1;
		}
		item = model.get_item (idx);
		deleted = false;
		return item != null;
	}

	public bool next () {
		if (started && (item == null)) {
			return false;
		}
		if (!started) {
			idx = 0;
			started = true;
		}
		else if (!deleted) {
			idx = idx + 1;
		}
		item = model.get_item (idx);
		deleted = false;
		return item != null;
	}

	public bool has_next () {
		return model.get_item ((!started ? 0 : deleted ? idx : idx + 1)) != null;
	}

	public bool has_previous () {
		return model.get_item ((!started ? (int) model.get_n_items () - 1 : idx - 1)) != null;
	}

	public int index () {
		return idx;
	}

	public int find_item (T _item) {
		while (next ()) {
			if (item == _item) {
				return index ();
			}
		}
		return -1;
	}

	public int find_first (MatchGenericItemFunc<T> match_func) {
		while (next ()) {
			if (match_func (item)) {
				return index ();
			}
		}
		return -1;
	}

	ListModel model;
	T item;
	protected int idx;
	protected bool started;
	protected bool deleted;
}

public class Gth.ListStoreIterator<T> : Gth.ListIterator<T> {
	public ListStoreIterator (ListStore _model) {
		base (_model);
		store = _model;
	}

	public void remove () {
		if (!started) {
			return;
		}
		store.remove ((uint) idx);
		deleted = true;
	}

	ListStore store;
}

public delegate bool MatchGenericItemFunc<T> (T item);
