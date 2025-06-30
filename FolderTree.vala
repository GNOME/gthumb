public class Gth.FolderTree : Gtk.Box {
	[CCode(notify = false)]
	public bool sidebar { get; construct; default = false; }
	public Gtk.ListView view;
	public FileData current_root;
	public FileData current_folder;
	public GenericList<FileData> current_children;
	public GenericList<FileData> roots;
	public bool show_hidden {
		get {
			return _show_hidden;
		}
		set {
			_show_hidden = value;
			reload ();
		}
	}
	public Gth.Sort sort;
	public string list_attributes;
	public bool single_root { get; construct set; default = false; }

	[Signal (action=true)]
	public signal void load (File location, LoadAction action);

	Gth.Job load_job;
	Queue<File> current_parents;
	Gtk.TreeListModel tree_model;
	Gtk.SingleSelection tree_selection_model;
	bool current_folder_as_root;

	private bool _show_hidden;

	const string FOLDER_ATTRIBUTES = STANDARD_ATTRIBUTES + "," + FileAttribute.STANDARD_SYMBOLIC_ICON;

	construct {
		current_root = null;
		current_folder = null;
		current_children = null;
		roots = new GenericList<FileData>();
		show_hidden = false;
		sort = { null, false };
		list_attributes = FOLDER_ATTRIBUTES;
		load_job = null;
		current_parents = new Queue<File>();
		current_folder_as_root = false;

		tree_model = new Gtk.TreeListModel (roots.model, false, false, (obj) => {
			var file_data = obj as Gth.FileData;
			if (file_data != null) {
				return file_data.get_children_model ().model;
			}
			return null;
		});

		tree_selection_model = new Gtk.SingleSelection (tree_model);
		tree_selection_model.autoselect = false;
		tree_selection_model.notify["selected"].connect ((obj, _param) => {
			if (!current_parents.is_empty ()) {
				// Ignore selection changes when still building the tree.
				return;
			}
			var local_model = obj as Gtk.SingleSelection;
			var row = tree_selection_model.selected_item as Gtk.TreeListRow;
			if (row != null) {
				var file_data = row.item as Gth.FileData;
				if (!FileData.equal (file_data, current_folder)) {
					load_subfolder (file_data.file);
					row.expanded = true;
				}
				else {
					row.expanded = true;
				}
			}
		});

		var factory = new Gtk.SignalListItemFactory ();
		factory.setup.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			list_item.child = new Gth.FolderTreeItem (this);
		});
		factory.teardown.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			list_item.child = null;
		});
		factory.bind.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			var file_item = list_item.child as Gth.FolderTreeItem;
			if (file_item != null) {
				var row = list_item.item as Gtk.TreeListRow;
				if (row != null) {
					var file_data = row.item as Gth.FileData;
					if (file_data != null) {
						file_item.bind (row, file_data);
					}
				}
			}
		});
		factory.unbind.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			var file_item = list_item.child as Gth.FolderTreeItem;
			if (file_item != null) {
				file_item.unbind ();
			}
		});

		var scrolled_window = new Gtk.ScrolledWindow ();
		scrolled_window.add_css_class ("undershoot-top");
		scrolled_window.add_css_class ("undershoot-bottom");
		if (!sidebar) {
			scrolled_window.add_css_class ("frame");
		}
		scrolled_window.vexpand = true;
		append (scrolled_window);

		view = new Gtk.ListView (tree_selection_model, factory);
		if (sidebar) {
			view.add_css_class ("navigation-sidebar");
		}
		view.hexpand = true;
		view.activate.connect ((position) => {
			var row = tree_model.get_row (position);
			if (row != null) {
				var file_data = row.item as Gth.FileData;
				if (file_data != null) {
					load (file_data.file, LoadAction.OPEN_AS_ROOT);
				}
			}
		});
		scrolled_window.set_child (view);
	}

	public void reload () {
		if (current_folder == null)
			return;
		collapse_folder_tree (); // Forces reloading of the folders.
		load_folder.begin (current_folder.file);
	}

	public async void load_folder (File location, LoadAction load_action = LoadAction.OPEN) throws Error {
		var source = app.get_source_for_file (location);
		if (source == null) {
			throw new IOError.FAILED (_("File type not supported"));
		}

		if (load_job != null) {
			load_job.cancel ();
		}

		var next_folder_as_root = (load_action == LoadAction.OPEN_AS_ROOT);

		bool changes_root;
		if (load_action == LoadAction.OPEN_AS_ROOT) {
			changes_root = true;
		}
		else if (load_action == LoadAction.OPEN_SUBFOLDER) {
			changes_root = false;
		}
		else if (single_root) {
			changes_root = true;
		}
		else {
			changes_root = (current_folder_as_root != next_folder_as_root) || (roots.model.n_items == 0);
		}

		current_folder_as_root = next_folder_as_root;

		if (load_action.changes_current_folder ()) {
			current_parents.clear ();
		}

		var local_job = app.new_job ("Load folder %s".printf (location.get_uri ()));
		load_job = local_job;
		try {
			// Mount the location if required.
			Gth.FileData nearest_root = Util.get_nearest_parent (location, app.roots);
			//stdout.printf (">> nearest_root: %s\n", (nearest_root != null) ? nearest_root.file.get_uri () : "(nil)");

			var try_again = false;
			if (nearest_root == null) {
				// Mount the enclosing volume.
				var mount_op = new Gtk.MountOperation (get_root () as Gtk.Window);
				yield location.mount_enclosing_volume (0, mount_op, local_job.cancellable);
				try_again = true;
			}
			else if (nearest_root.info.get_file_type () == FileType.MOUNTABLE) {
				var volume = nearest_root.info.get_attribute_object (VOLUME_ATTRIBUTE) as Volume;
				if (volume != null) {
					var mount_op = new Gtk.MountOperation (get_root () as Gtk.Window);
					yield volume.mount (0, mount_op, local_job.cancellable);
					var mount = volume.get_mount ();
					if (mount == null) {
						throw new IOError.NOT_MOUNTED (_("Location not available"));
					}
				}
				else {
					var mount_op = new Gtk.MountOperation (get_root () as Gtk.Window);
					yield nearest_root.file.mount_mountable (0, mount_op, local_job.cancellable);
				}
				try_again = true;
			}
			if (try_again) {
				// TODO: update the window bookmarks as well.
				yield app.update_roots ();
				nearest_root = Util.get_nearest_parent (location, app.roots);
				if ((nearest_root == null) || (nearest_root.info.get_file_type () != FileType.DIRECTORY)) {
					throw new IOError.NOT_MOUNTED (_("Location not available"));
				}
			}

			// Read the requested location metadata.
			var file_data = yield source.read_metadata (location, "*", local_job.cancellable);

			// List the location children.
			var all_attributes = Util.concat_attributes (list_attributes, file_data.get_sort_attributes ());
			var children = yield source.list_children (location, all_attributes, local_job.cancellable);

			if (load_action.changes_current_folder ()) {
				var parent = location;
				while (parent != null) {
					//stdout.printf (">> add parent: %s\n", parent.get_uri ());
					current_parents.push_head (parent);
					if (parent.equal (nearest_root.file)) {
						break;
					}
					parent = parent.get_parent ();
				}
			}

			if (load_action == LoadAction.OPEN_AS_ROOT) {
				nearest_root = file_data;
			}

			if (load_action.changes_current_folder ()) {
				if (location.equal (nearest_root.file)) {
					current_root = file_data;
				}
				else {
					var root = current_parents.peek_head ();
					current_root = yield source.read_metadata (root, "*", local_job.cancellable);
				}
				current_folder = file_data;
				current_children = children;
			}

			//if (load_action != LoadAction.OPEN_FROM_HISTORY) {
			//	history.add (current_folder.file);
			//}
			if (changes_root) {
				update_folder_tree ();
			}
			if (load_action.changes_current_folder ()) {
				expand_tree_to_current_folder ();
			}
		}
		catch (Error error) {
			stdout.printf ("ERROR: %s\n", error.message);
			local_job.error = error;
		}
		local_job.done ();
		if (load_job == local_job) {
			load_job = null;
		}
		if (local_job.error != null) {
			throw local_job.error;
		}
	}

	void load_subfolder (File location) {
		load (location, LoadAction.OPEN_SUBFOLDER);
	}

	public Gth.Job? list_subfolders (FileData file_data) {
		//stdout.printf ("> LIST SUBFOLDERS %s\n", file_data.file.get_uri ());

		if (FileData.equal (file_data, current_folder)) {
			set_file_data_children (file_data, current_children);
			return null;
		}

		var source = app.get_source_for_file (file_data.file);
		if (source == null) {
			//throw new IOError.FAILED (_("File type not supported"));
			return null;
		}
		var local_job = app.new_job ("List subfolders for %s".printf (file_data.file.get_uri ()));
		source.list_children.begin (file_data.file, FOLDER_ATTRIBUTES, local_job.cancellable, (_obj, res) => {
			try {
				var all_children = source.list_children.end (res);
				set_file_data_children (file_data, all_children);
			}
			catch (Error error) {
			}
			local_job.done ();
		});
		return local_job;
	}

	public bool is_loading () {
		return (load_job != null) && load_job.is_running ();
	}

	public void set_sort_order (string name, bool inverse) {
		sort = { name, inverse };
		unowned var sort_info = app.get_folder_sorter_by_id (sort.name);
		var iter = new TreeIterator<Gtk.TreeListRow> (tree_model);
		while (iter.next ()) {
			var row = iter.get ();
			var file_data = row.item as Gth.FileData;
			if (file_data != null) {
				var file_model = file_data.get_children_model ();
				file_model.model.sort ((a, b) => {
					var result = sort_info.cmp_func ((FileData) a, (FileData) b);
					if (sort.inverse)
						result *= -1;
					return result;
				});
			}
		}
		select_current_folder ();
	}

	void collapse_folder_tree () {
		var iter = new TreeIterator<Gtk.TreeListRow> (tree_model);
		while (iter.next ()) {
			var row = iter.get ();
			row.expanded = false;
		}
	}

	public Gtk.TreeListRow? get_file_row (File file, out int position = null) {
		var iter = new TreeIterator<Gtk.TreeListRow> (tree_model);
		while (iter.next ()) {
			var row = iter.get ();
			var file_data = row.item as Gth.FileData;
			if ((file_data != null) && (file_data.file.equal (file))) {
				position = iter.index ();
				return row;
			}
		}
		position = -1;
		return null;
	}

	public File? get_selected () {
		var row = tree_selection_model.selected_item as Gtk.TreeListRow;
		if (row == null) {
			return null;
		}
		var file_data = row.item as Gth.FileData;
		if (file_data == null) {
			return null;
		}
		return file_data.file;
	}

	void update_folder_tree () {
		roots.model.remove_all ();
		if (current_folder_as_root || single_root) {
			if (current_root != null) {
				roots.model.append (current_root);
			}
		}
		else {
			foreach (unowned var root in app.roots) {
				var local_root = new FileData.copy (root);
				roots.model.append (local_root);
			}
		}
	}

	void expand_tree_to_current_folder () {
		var root = current_parents.peek_head ();
		if (root != null) {
			var root_row = get_file_row (root);
			if (root_row != null) {
				if (root_row.expanded) {
					expand_next_parent_for_current_folder ();
				}
				else {
					root_row.expanded = true;
				}
			}
		}
	}

	void expand_next_parent_for_current_folder () {
		current_parents.pop_head ();
		var next_parent = current_parents.peek_head ();
		if (next_parent == null) {
			//stdout.printf ("  next_parent: (null)\n");
			select_current_folder ();
		}
		else if ((current_folder != null) && (current_folder.file.equal (next_parent))) {
			//stdout.printf ("  next_parent == current_folder\n");
			current_parents.pop_head ();
			select_current_folder ();
		}
		else {
			//stdout.printf ("  next_parent: '%s'\n", next_parent.get_uri ());
			var next_parent_row = get_file_row (next_parent);
			if (next_parent_row != null) {
				if (next_parent_row.expanded) {
					expand_next_parent_for_current_folder ();
				}
				else {
					next_parent_row.expanded = true;
				}
			}
		}
	}

	void set_file_data_children (FileData file_data, GenericList<FileData> all_children) {
		// Filter the folders
		var folders = new GenericList<FileData> ();
		foreach (unowned var file in all_children) {
			if (file.info.get_file_type () == FileType.DIRECTORY) {
				if (_show_hidden || !file.info.get_is_hidden ()) {
					folders.model.append (file);
				}
			}
		}

		// Sort the folders
		unowned var sort_info = app.get_folder_sorter_by_id (sort.name);
		folders.model.sort ((a, b) => {
			var result = sort_info.cmp_func ((FileData) a, (FileData) b);
			if (sort.inverse)
				result *= -1;
			return result;
		});

		// Update file_data
		var file_model = file_data.get_children_model ();
		file_model.model.remove_all ();
		foreach (unowned var file in folders) {
			file_model.model.append (file);
		}
		file_data.children_loaded = true;

		// Continue building the folder tree.
		//stdout.printf ("set_file_data_children: '%s'\n", file_data.file.get_uri ());
		//stdout.printf ("  current_parents: %p\n", current_parents);
		var top_parent = current_parents.peek_head ();
		//stdout.printf ("  top_parent: %s\n", (top_parent != null) ? top_parent.get_uri () : "(null)");
		if ((top_parent != null) && top_parent.equal (file_data.file)) {
			expand_next_parent_for_current_folder ();
		}
	}

	void select_current_folder () {
		if (current_folder == null)
			return;
		int position;
		var current_row = get_file_row (current_folder.file, out position);
		//stdout.printf ("CURRENT FOLDER POS: %d\n", position);
		if (position >= 0) {
			view.scroll_to ((uint) position, Gtk.ListScrollFlags.SELECT | Gtk.ListScrollFlags.FOCUS, null);
		}
	}
}
