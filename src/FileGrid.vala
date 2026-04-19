[GtkTemplate (ui = "/org/gnome/gthumb/ui/file-grid.ui")]
public class Gth.FileGrid : Gtk.Box {
	public IterableList<FileData> visible_files;
	public string[] thumbnail_attributes_v;
	public bool reordering;
	public bool is_reorderable;
	public unowned Gtk.PopoverMenu file_context_menu;
	public Gth.Thumbnailer thumbnailer {
		set {
			_thumbnailer = value;
			_thumbnailer.get_next_file_func = get_next_file_for_thumbnailer;
		}
		get {
			return _thumbnailer;
		}
	}
	public bool filename_as_tooltip { get; set; default = false; }

	[GtkChild] public unowned Gtk.GridView view;

	public uint thumbnail_size {
		get { return _thumbnail_size; }
		set {
			_thumbnail_size = value;
			if (_thumbnailer != null) {
				_thumbnailer.requested_size = _thumbnail_size;
				foreach (unowned Gtk.ListItem item in binded_grid_items) {
					unowned var file_item = item.child as Gth.FileGridItem;
					file_item.set_size (_thumbnail_size);
				}
				_thumbnailer.queue_load_next ();
			}
		}
	}

	public void set_model (ListModel model, bool single_selection = false) {
		visible_files = new IterableList<FileData> (model);
		if (single_selection) {
			view.model = new Gtk.SingleSelection (model);
		}
		else {
			view.model = new Gtk.MultiSelection (model);
		}
	}

	public int get_file_position (File file) {
		var iter = visible_files.iterator ();
		return iter.find_first ((file_data) => file_data.file.equal (file));
	}

	public void open_file_context_menu (Gth.FileGridItem item, int x, int y) {
		if (!is_selected (item.file_data.file)) {
			select_file (item.file_data.file);
		}
		if (file_context_menu != null) {
			file_context_menu.pointing_to = { x, y, 18, 18 };
			file_context_menu.popup ();
		}
	}

	public void select_file (File file, SelectFile flags = SelectFile.DEFAULT) {
		var iter = visible_files.iterator ();
		var pos = iter.find_first ((file_data) => file_data.file.equal (file));
		select_position (pos, flags);
	}

	public void select_position (int position, SelectFile flags = SelectFile.DEFAULT) {
		if (position < 0) {
			return;
		}
		if (SelectFile.SCROLL_TO_FILE in flags) {
			view.scroll_to ((uint) position, Gtk.ListScrollFlags.SELECT | Gtk.ListScrollFlags.FOCUS, null);
		}
		else {
			view.model.select_item ((uint) position, true);
		}
	}

	public void select_files (GenericList<File> list) {
		var first = true;
		foreach (unowned var file in list) {
			var position = get_file_position (file);
			if (position >= 0) {
				if (first) {
					select_position (position, SelectFile.SCROLL_TO_FILE);
					first = false;
				}
				else {
					view.model.select_item ((uint) position, false);
				}
			}
		}
	}

	public bool is_selected (File file) {
		var selection = view.model.get_selection ();
		for (int64 idx = 0; idx < selection.get_size (); idx++) {
			var pos = selection.get_nth ((uint) idx);
			var selected_file = view.model.get_item (pos) as FileData;
			if (selected_file.file.equal (file)) {
				return true;
			}
		}
		return false;
	}

	public uint get_selected_position () {
		var selected = view.model.get_selection ();
		if (selected.get_size () != 1) {
			return uint.MAX;
		}
		return selected.get_nth (0);
	}

	public FileData? get_selected_file_data () {
		var pos = get_selected_position ();
		if (pos == uint.MAX) {
			return null;
		}
		var file_data = view.model.get_item (pos) as FileData;
		return file_data;
	}

	public File? get_selected_file () {
		var file_data = get_selected_file_data ();
		return (file_data != null) ? file_data.file : null;
	}

	public GenericList<File> get_selected_files () {
		var files = new GenericList<File> ();
		var selection = view.model.get_selection ();
		var n_selected = selection.get_size ();
		for (var i = 0; i < n_selected; i++) {
			var pos = selection.get_nth (i);
			var selected_file = view.model.get_item (pos) as FileData;
			files.model.append (selected_file.file);
		}
		return files;
	}

	public GenericList<FileData> get_selected_file_data_list () {
		var files = new GenericList<FileData> ();
		var selection = view.model.get_selection ();
		var n_selected = selection.get_size ();
		for (var i = 0; i < n_selected; i++) {
			var pos = selection.get_nth (i);
			var selected_file = view.model.get_item (pos) as FileData;
			files.model.append (selected_file);
		}
		return files;
	}

	public GenericList<FileData> get_file_data_list () {
		var files = new GenericList<FileData> ();
		foreach (var file_data in visible_files) {
			files.model.append (file_data);
		}
		return files;
	}

	public Gtk.ListItem? get_item_at (double x, double y, out DropSide? side) {
		Graphene.Point point = { (float) x, (float) y };
		var top = 0;
		var bottom = view.vadjustment.get_page_size ();
		Gtk.ListItem closest_item = null;
		float min_distance = 0f;
		side = DropSide.NONE;
		foreach (unowned Gtk.ListItem item in binded_grid_items) {
			if (!item.child.get_mapped ()) {
				continue;
			}
			Graphene.Rect bounds;
			if (item.child.compute_bounds (view, out bounds)) {
				if (bounds.origin.x < 0) {
					continue;
				}

				// stdout.printf ("> (%f,%f)[%f,%f] <=> VIEW: [%f,%f]\n",
				// 	bounds.origin.x, bounds.origin.y,
				// 	bounds.size.width, bounds.size.height,
				// 	top, bottom);

				if ((bounds.origin.y < top - bounds.size.height) || (bounds.origin.y > bottom)) {
					continue;
				}

				// stdout.printf ("> (%f,%f)[%f,%f] <=> POINT: [%f,%f]\n",
				// 	bounds.origin.x, bounds.origin.y,
				// 	bounds.size.width, bounds.size.height,
				// 	point.x, point.y);

				if ((point.y < bounds.origin.y) || (point.y > bounds.origin.y + bounds.size.height)) {
					continue;
				}

				if ((point.x >= bounds.origin.x) && (point.x <= bounds.origin.x + bounds.size.width)) {
					side = (point.x < bounds.origin.x + (bounds.size.width / 2)) ? DropSide.LEFT : DropSide.RIGHT;
					return item;
				}

				if (point.x < bounds.origin.x) {
					var distance = bounds.origin.x - point.x;
					if ((closest_item == null) || (distance < min_distance)) {
						closest_item = item;
						min_distance = distance;
						side = DropSide.LEFT;
					}
				}
				else {
					var distance = point.x - (bounds.origin.x + bounds.size.width);
					if ((closest_item == null) || (distance < min_distance)) {
						closest_item = item;
						min_distance = distance;
						side = DropSide.RIGHT;
					}
				}
			}
		}
		return closest_item;
	}

	public void stop_thumbnailer () {
		_thumbnailer.cancel ();
		_thumbnailer.set_active (false);
	}

	public void start_thumbnailer () {
		_thumbnailer.set_active (true);
		_thumbnailer.queue_load_next ();
	}

	public void update_thumbnails (GenericArray<FileData> files) {
		if (files.length == 0) {
			return;
		}
		if (_thumbnailer != null) {
			foreach (var file in files) {
				_thumbnailer.add (file);
			}
			_thumbnailer.queue_load_next ();
		}
	}

	public void update_thumbnail (FileData file) {
		if (_thumbnailer != null) {
			_thumbnailer.add (file);
			_thumbnailer.queue_load_next ();
		}
	}

	public Thumbnailer.Size get_thumbnailer_cache_size () {
		return (_thumbnailer != null) ? _thumbnailer.cache_size : Thumbnailer.Size.NORMAL;
	}

	Gth.FileData? get_next_file_for_thumbnailer () {
		// stdout.printf ("\n>>>> get_next_file_for_thumbnailer\n\n");
		// First visible item.
		var top = 0;
		var bottom = view.vadjustment.get_page_size ();
		foreach (unowned Gtk.ListItem item in binded_grid_items) {
			if (!item.child.get_mapped ()) {
				continue;
			}
			Graphene.Rect bounds;
			if (item.child.compute_bounds (view, out bounds)) {
				if (bounds.origin.x < 0) {
					continue;
				}

				var file_data = item.item as FileData;
				if (file_data.has_thumbnail ()) {
					continue;
				}
				if (_thumbnailer.already_added (file_data)) {
					continue;
				}

				//stdout.printf ("> (%f,%f)[%f,%f] (state: %s) <=> [%f,%f]\n",
				//	bounds.origin.x, bounds.origin.y,
				//	bounds.size.width, bounds.size.height,
				//	file_data.thumbnail_state.to_string (),
				//	top, bottom);

				if ((bounds.origin.y < top - bounds.size.height) || (bounds.origin.y > bottom)) {
					continue;
				}

				//stdout.printf ("> y: %f, height %f (%s) <=> [%f,%f]\n",
				//	bounds.origin.y,
				//	bounds.size.height,
				//	file_data.file.get_basename (),
				//	top, bottom);

				return file_data;
			}
		}
		// First binded item.
		foreach (unowned Gtk.ListItem item in binded_grid_items) {
			var file_data = item.item as FileData;
			if (file_data.has_thumbnail ()) {
				continue;
			}
			if (_thumbnailer.already_added (file_data)) {
				continue;
			}
			return file_data;
		}
		return null;
	}

	void init_thumbnailer () {
		if (_thumbnailer != null) {
			_thumbnailer.queue_load_next ();
		}
	}

	void vadjustment_changed () {
		if (_thumbnailer != null) {
			_thumbnailer.queue_load_next ();
		}
	}

	construct {
		reordering = false;
		is_reorderable = false;
		_thumbnailer = null;
		file_context_menu = null;
		thumbnail_attributes_v = {};
		_thumbnail_size = DEFAULT_THUMBNAIL_SIZE;
		visible_files = null;
		binded_grid_items = new GenericArray<Gtk.ListItem> ();
		realize.connect (() => init_thumbnailer ());

		var factory = new Gtk.SignalListItemFactory ();
		factory.setup.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			list_item.child = new Gth.FileGridItem (this);
		});
		factory.teardown.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			list_item.child = null;
		});
		factory.bind.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			var file_item = list_item.child as Gth.FileGridItem;
			var file_data = list_item.item as FileData;
			if (file_data != null) {
				file_item.bind (file_data);
				if (file_item.parent != null) {
					file_item.parent.halign = Gtk.Align.CENTER;
				}
				binded_grid_items.add (list_item);
				if (_thumbnailer != null) {
					_thumbnailer.queue_load_next ();
					if (binded_grid_items.length < 50) {
						_thumbnailer.add (file_data);
					}
				}
			}
		});
		factory.unbind.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			var file_item = list_item.child as Gth.FileGridItem;
			if (file_item != null) {
				file_item.unbind ();
				var file_data = list_item.item as FileData;
				if (file_data != null) {
					if (_thumbnailer != null) {
						_thumbnailer.remove (file_data);
					}
					file_data.remove_thumbnail ();
				}
			}
			binded_grid_items.remove (list_item);
		});
		view.factory = factory;

		view.vadjustment.changed.connect (() => vadjustment_changed ());
		view.vadjustment.value_changed.connect (() => vadjustment_changed ());
	}

	GenericArray<Gtk.ListItem> binded_grid_items;
	Gth.Thumbnailer _thumbnailer;
	uint _thumbnail_size;

	const uint DEFAULT_THUMBNAIL_SIZE = 256;
}
