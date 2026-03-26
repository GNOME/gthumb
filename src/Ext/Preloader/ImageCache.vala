public class Gth.ImageCache {
	public ImageCache (uint _max_size = DEFAULT_MAX_SIZE) {
		max_size = _max_size;
		list = new GenericArray<CacheItem?> ();
	}

	public void add (File file, Image image) {
		if (touch (file)) {
			return;
		}
		while (list.length >= max_size) {
			list.remove_index (0);
		}
		list.add (CacheItem (file, image));
	}

	public int index_of (File file) {
		for (var idx = 0; idx < list.length; idx++) {
			unowned var item = list[idx];
			if (item.file.equal (file)) {
				return idx;
			}
		}
		return -1;
	}

	public bool contains (File file) {
		return index_of (file) >= 0;
	}

	public Image? get (File file) {
		var idx = index_of (file);
		return (idx >= 0) ? list[idx].image : null;
	}

	// Move at the end if found.
	public bool touch (File file) {
		var idx = index_of (file);
		if (idx < 0) {
			return false;
		}
		var item = list[idx];
		list.remove_index (idx);
		list.add (item);
		return true;
	}

	public void remove (File file) {
		var idx = index_of (file);
		if (idx >= 0) {
			list.remove_index (idx);
		}
	}

	public void clear () {
		list.length = 0;
	}

	public void print () {
		stdout.printf ("> CACHE:\n");
		foreach (unowned var item in list) {
			stdout.printf ("  - %s\n", item.file.get_uri ());
		};
	}

	struct CacheItem {
		File file;
		Image image;

		public CacheItem (File _file, Image _image) {
			file = _file;
			image = _image;
		}
	}

	uint max_size;
	GenericArray<CacheItem?> list;

	public const uint DEFAULT_MAX_SIZE = 3;
}
