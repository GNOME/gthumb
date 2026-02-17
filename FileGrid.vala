[GtkTemplate (ui = "/app/gthumb/gthumb/ui/file-grid.ui")]
public class Gth.FileGrid : Gtk.Box {
	[GtkChild] public unowned Gtk.GridView view;
	public string[] thumbnail_attributes_v;
	public Gth.Thumbnailer thumbnailer;
	public bool reordering;
	public bool is_reorderable;
	unowned Gtk.PopoverMenu file_context_menu;

	public uint thumbnail_size {
		get { return _thumbnail_size; }
		set {
			if (value == _thumbnail_size) {
				return;
			}
			_thumbnail_size = value;
			if (thumbnailer != null) {
				thumbnailer.requested_size = _thumbnail_size;
				foreach (unowned Gtk.ListItem item in binded_grid_items) {
					unowned var file_item = item.child as Gth.FileGridItem;
					file_item.set_size (_thumbnail_size);
				}
				thumbnailer.queue_load_next ();
			}
		}
	}

	public IterableList<FileData> visible_files;

	public void set_model (ListModel model, bool single_selection = false) {
		visible_files = new IterableList<FileData> (model);
		if (single_selection) {
			view.model = new Gtk.SingleSelection (model);
		}
		else {
			view.model = new Gtk.MultiSelection (model);
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

	public void open_file_context_menu (Gth.FileGridItem item, int x, int y) {
		if (!is_selected (item.file_data.file)) {
			select_file (item.file_data.file);
		}
		Graphene.Point p = Graphene.Point.zero ();
		item.compute_point (this, Graphene.Point.zero (), out p);
		if (file_context_menu != null) {
			file_context_menu.pointing_to = { (int) p.x + x, (int) p.y + y, 1, 12 };
			file_context_menu.popup ();
		}
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

	Gtk.ListItem? get_item_at (double x, double y, out DropSide side) {
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
				if (thumbnailer.already_added (file_data)) {
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
			if (thumbnailer.already_added (file_data)) {
				continue;
			}
			return file_data;
		}
		return null;
	}

	void init_thumbnailer () {
		if (thumbnailer == null) {
			if (root is Gth.Window) {
				var window = root as Gth.Window;
				thumbnailer = new Thumbnailer.for_window (window);
				thumbnailer.get_next_file_func = get_next_file_for_thumbnailer;
				thumbnailer.requested_size = _thumbnail_size;
				thumbnailer.queue_load_next ();
			}
		}
	}

	construct {
		reordering = false;
		is_reorderable = false;
		thumbnailer = null;
		file_context_menu = null;
		thumbnail_attributes_v = {};
		_thumbnail_size = DEFAULT_THUMBNAIL_SIZE;
		visible_files = null;
		binded_grid_items = new GenericArray<Gtk.ListItem> ();
		realize.connect (() => init_thumbnailer ());

		var factory = new Gtk.SignalListItemFactory ();
		factory.setup.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			list_item.child = new Gth.FileGridItem (this, thumbnail_attributes_v);
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
				if (thumbnailer != null) {
					thumbnailer.queue_load_next ();
					if (binded_grid_items.length < 50) {
						thumbnailer.add (file_data);
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
					if (thumbnailer != null) {
						thumbnailer.remove (file_data);
					}
					file_data.remove_thumbnail ();
				}
			}
			binded_grid_items.remove (list_item);
		});
		view.factory = factory;
	}

	GenericArray<Gtk.ListItem> binded_grid_items;
	uint _thumbnail_size;

	const uint DEFAULT_THUMBNAIL_SIZE = 256;
}
