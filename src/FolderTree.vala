public class Gth.FolderTree : Gtk.Box {
	[CCode(notify = false)]
	public bool sidebar { get; construct; default = false; }
	public Gtk.ListView view;
	public FileData current_root;
	public FileData current_folder;
	public FileSource current_source;
	public GenericList<FileData> current_children;
	public GenericList<FileData> visible_roots;
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
	public weak JobQueue job_queue;
	public MenuModel menu_model {
		set {
			if (context_menu != null) {
				context_menu.menu_model = value;
			}
		}
		get {
			return (context_menu != null) ? context_menu.menu_model : null;
		}
	}
	public FileData context_file;
	public bool skip_nochild_folders = false; // Used to show only libraries when moving a catalog.
	public LoadState load_state = LoadState.NONE;
	public bool loading { get; set; default = false; }

	[Signal (action=true)]
	public signal void load (FileData location, LoadAction action);

	public signal void before_context_menu_popup (FileData file_data);

	Gth.Job load_job;
	Queue<File> expand_parents;
	FileData last_expanded;
	public Gtk.TreeListModel tree_model;
	Gtk.SingleSelection tree_selection_model;
	bool current_folder_as_root;
	Gtk.PopoverMenu context_menu;

	private bool _show_hidden;

	const string FOLDER_ATTRIBUTES = STANDARD_ATTRIBUTES + "," + FileAttribute.STANDARD_SYMBOLIC_ICON;

	public void reload () {
		if (current_folder == null)
			return;
		collapse_folder_tree (); // Forces reloading of the folders.
		load_folder.begin (current_folder.file);
	}

	public bool get_is_loading () {
		return (load_job != null);
	}

	public async void load_folder (File location, LoadAction load_action = LoadAction.OPEN, Job? external_job = null) throws Error {
		try {
			load_state = LoadState.LOADING;
			yield load_folder_private (location, load_action, external_job);
			load_state = LoadState.SUCCESS;
		}
		catch (Error error) {
			load_state = LoadState.ERROR;
			throw error;
		}
	}

	async void load_folder_private (File location, LoadAction load_action = LoadAction.OPEN, Job? external_job = null) throws Error {
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
		else if ((load_action == LoadAction.OPEN_SUBFOLDER) || (load_action == LoadAction.OPEN_NEW_FOLDER)) {
			changes_root = false;
		}
		else if (single_root) {
			changes_root = true;
		}
		else {
			changes_root = (current_folder_as_root != next_folder_as_root) || (visible_roots.model.n_items == 0);
		}

		current_folder_as_root = next_folder_as_root;

		if (current_folder != null) {
			watched_files.remove (current_folder.file);
		}

		if (load_action.changes_current_folder ()) {
			expand_parents.clear ();
			last_expanded = null;
		}

		loading = true;

		var job_is_local = (external_job == null);
		var local_job = external_job;
		if (local_job == null) {
			local_job = job_queue.new_job (_("Loading %s").printf (location.get_uri ()),
				JobFlags.DEFAULT,
				"folder-symbolic");
			load_job = local_job;
		}
		try {
			var nearest_root = yield app.devices.ensure_mounted (location, get_root () as Gtk.Window, local_job.cancellable);

			// Read the requested location metadata.
			var file_data = yield source.read_metadata (location, "*", local_job.cancellable);

			// List the location children.
			var all_attributes = Util.concat_attributes (list_attributes, file_data.get_sort_attributes ());
			var children = yield source.list_children (location, all_attributes, local_job.cancellable);

			if (load_action == LoadAction.OPEN_AS_ROOT) {
				nearest_root = file_data;
			}

			if (load_action.changes_current_folder ()) {
				if (location.equal (nearest_root.file)) {
					current_root = file_data;
				}
				else {
					var root = nearest_root.file;
					current_root = yield source.read_metadata (root, "*", local_job.cancellable);
				}
				current_folder = file_data;
				current_children.copy (children);
				current_source = source;

				if (load_action == LoadAction.OPEN_NEW_FOLDER) {
					// Expand from parent.
					var parent = location.get_parent ();
					if (parent != null) {
						expand_parents.push_head (location);
						expand_parents.push_head (parent);
						var row = get_file_row (parent);
						if (row != null) {
							last_expanded = row.item as Gth.FileData;
							if (last_expanded != null) {
								last_expanded.children_state = ChildrenState.NONE;
							}
						}
					}
				}
				else {
					// Expand from nearest_root.
					var parent = location;
					while (parent != null) {
						expand_parents.push_head (parent);
						if (parent.equal (nearest_root.file)) {
							break;
						}
						parent = parent.get_parent ();
					}
				}
			}

			if (changes_root) {
				update_folder_tree ();
			}
			if (load_action.changes_current_folder ()) {
				watched_files.add (current_folder.file);
				expand_tree_to_current_folder ();
			}
		}
		catch (Error error) {
			local_job.error = error;
			// Watch again the current folder.
			if (load_action.changes_current_folder ()) {
				if (current_folder != null) {
					watched_files.add (current_folder.file);
				}
			}
		}
		if (job_is_local) {
			local_job.done ();
			if (load_job == local_job) {
				load_job = null;
			}
		}
		if (!building_the_tree ()) {
			loading = false;
		}
		if (local_job.error != null) {
			throw local_job.error;
		}
	}

	void load_subfolder (FileData location) {
		load (location, LoadAction.OPEN_SUBFOLDER);
	}

	public void list_subfolders (FileData file_data) {
		// stdout.printf ("> LIST SUBFOLDERS %s\n", file_data.file.get_uri ());

		if (file_data.children_state != ChildrenState.NONE) {
			return;
		}

		if (FileData.equal (file_data, current_folder)) {
			set_file_data_children (file_data, current_children);
			return;
		}

		var source = app.get_source_for_file (file_data.file);
		if (source == null) {
			//throw new IOError.FAILED (_("File type not supported"));
			return;
		}

		if (load_job != null) {
			load_job.cancel ();
		}

		file_data.children_state = ChildrenState.LOADING;
		var local_job = job_queue.new_job (_("Loading %s").printf (file_data.get_display_name ()),
			JobFlags.DEFAULT,
			"folder-symbolic");
		load_job = local_job;
		loading = true;
		source.list_children.begin (file_data.file, FOLDER_ATTRIBUTES, local_job.cancellable, (_obj, res) => {
			try {
				var all_children = source.list_children.end (res);
				set_file_data_children (file_data, all_children);
			}
			catch (Error error) {
				file_data.children_state = ChildrenState.NONE;
			}
			finally {
				local_job.done ();
				if (load_job == local_job) {
					load_job = null;
					loading = false;
				}
			}
		});
	}

	public bool is_loading () {
		return (load_job != null) && load_job.is_running ();
	}

	public void set_sort_order (string name, bool inverse) {
		sort = { name, inverse };
		reload ();
	}

	void collapse_folder_tree () {
		var iter = new TreeIterator<Gth.FileData> (tree_model);
		while (iter.next ()) {
			var row = iter.get_row ();
			row.expanded = false;
		}
	}

	public FileData? get_file_data (File file) {
		var iter = new TreeIterator<Gth.FileData> (tree_model);
		while (iter.next ()) {
			var file_data = iter.get_data ();
			if ((file_data != null) && (file_data.file.equal (file))) {
				return file_data;
			}
		}
		return null;
	}

	public Gtk.TreeListRow? get_file_row (File? file, out int position = null) {
		if (file == null) {
			position = -1;
			return null;
		}
		var iter = new TreeIterator<Gth.FileData> (tree_model);
		while (iter.next ()) {
			var file_data = iter.get_data ();
			if ((file_data != null) && (file_data.file.equal (file))) {
				position = iter.index ();
				return iter.get_row ();
			}
		}
		position = -1;
		return null;
	}

	public File? get_file_at_position (uint pos) {
		var row = tree_model.get_row (pos);
		if (row == null) {
			return null;
		}
		var file_data = row.item as Gth.FileData;
		if (file_data == null) {
			return null;
		}
		return file_data.file;
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

	public FileData? get_selected_file_data () {
		var row = tree_selection_model.selected_item as Gtk.TreeListRow;
		if (row == null) {
			return null;
		}
		return row.item as Gth.FileData;
	}

	public void watch_directory (FileData file_data, bool watch) {
		if (watch) {
			watched_files.add (file_data.file);
		}
		else {
			watched_files.remove (file_data.file);
		}
	}

	void update_folder_tree () {
		visible_roots.model.remove_all ();
		watched_files.remove_all ();
		if (current_folder_as_root || single_root) {
			if (current_root != null) {
				visible_roots.model.append (current_root);
			}
		}
		else {
			foreach (unowned var root in roots) {
				var local_root = new FileData.copy (root);
				visible_roots.model.append (local_root);
			}
		}
	}

	void expand_tree_to_current_folder () {
		if (last_expanded == null) {
			if (current_root == null) {
				return;
			}
			var root = expand_parents.peek_head ();
			if (root == null) {
				return;
			}
			if (!current_root.file.equal (root)) {
				return;
			}
			last_expanded = current_root;
		}
		var row = get_file_row (last_expanded.file);
		if (row == null) {
			after_expand_tree ();
			return;
		}
		if (row.expanded && (last_expanded.children_state == ChildrenState.LOADED)) {
			expand_next_parent_for_current_folder ();
		}
		else {
			list_subfolders (last_expanded);
		}
	}

	void after_expand_tree () {
		expand_parents.clear ();
		last_expanded = null;
		queue_select_current_folder ();
		loading = false;
	}

	void expand_next_parent_for_current_folder () {
		expand_parents.pop_head ();
		var next_parent = expand_parents.peek_head ();
		if (next_parent == null) {
			after_expand_tree ();
		}
		else {
			var iter = last_expanded.children.iterator ();
			var file_data = iter.find_first_item ((file_data) => file_data.file.equal (next_parent));
			if (file_data != null) {
				last_expanded = file_data;
				list_subfolders (file_data);
			}
			else {
				after_expand_tree ();
			}
		}
	}

	bool expand_file_row (File file) {
		int position;
		var row = get_file_row (file, out position);
		if (row == null) {
			return false;
		}
		row.expanded = true;
		return true;
	}

	void set_file_data_children (FileData file_data, GenericList<FileData> all_children) {
		// Filter the folders
		var folders = new GenericList<FileData> ();
		foreach (unowned var file in all_children) {
			if (file.info.get_file_type () == FileType.DIRECTORY) {
				if (!_show_hidden && file.info.get_is_hidden ()) {
					continue;
				}
				if (skip_nochild_folders && file.info.get_attribute_boolean ("gthumb::no-child")) {
					continue;
				}
				folders.model.append (file);
			}
		}

		// Sort the folders
		unowned var sort_info = app.get_folder_sorter_by_id (sort.name);
		folders.model.sort ((a, b) => {
			var result = sort_info.cmp_func ((FileData) a, (FileData) b);
			if (sort.inverse) {
				result = - result;
			}
			return result;
		});

		// Update file_data
		var file_model = file_data.get_children_model ();
		file_model.model.remove_all ();
		foreach (unowned var file in folders) {
			file_model.model.append (file);
		}
		file_data.children_state = ChildrenState.LOADED;
		if (expand_file_row (file_data.file)) {
			watch_directory (file_data, true);
		}

		// Continue building the folder tree.
		if ((last_expanded != null) && last_expanded.file.equal (file_data.file)) {
			expand_next_parent_for_current_folder ();
		}
	}

	uint select_id = 0;
	bool scroll_to_selected = false;

	void queue_select_current_folder (bool _scroll_to_selected = true) {
		scroll_to_selected = _scroll_to_selected;
		if (select_id != 0) {
			Source.remove (select_id);
		}
		select_id = Util.after_next_rearrange (() => {
			select_id = 0;
			select_current_folder ();
		});
	}

	public bool select_position (int position) {
		if ((position < 0) || (position >= tree_model.n_items)) {
			return false;
		}
		view.scroll_to ((uint) position, Gtk.ListScrollFlags.SELECT | Gtk.ListScrollFlags.FOCUS, null);
		return true;
	}

	bool building_the_tree () {
		return !expand_parents.is_empty ();
	}

	void select_current_folder () {
		if ((current_folder == null) || building_the_tree ()) {
			return;
		}
		int position = 0;
		if (get_file_row (current_folder.file, out position) != null) {
			view.scroll_to ((uint) position, Gtk.ListScrollFlags.SELECT | Gtk.ListScrollFlags.FOCUS, null);
		}
	}

	public void open_context_menu (FolderTreeItem item, int x, int y) {
		if (context_menu.menu_model == null) {
			return;
		}

		before_context_menu_popup (item.file_data);
		context_file = item.file_data;

		Graphene.Point p = Graphene.Point.zero ();
		item.compute_point (this, Graphene.Point.zero (), out p);
		context_menu.pointing_to = { (int) p.x + x, (int) p.y + y, 1, 12 };
		context_menu.popup ();
		context_menu_visible = true;

		// Select the item without loading the content.
		selected_before_context_menu = (int) tree_selection_model.selected;
		int position;
		get_file_row (context_file.file, out position);
		tree_selection_model.set_selected (position);
	}

	construct {
		current_root = null;
		current_folder = null;
		current_source = null;
		current_children = new GenericList<FileData>();
		roots = app.roots;
		visible_roots = new GenericList<FileData>();
		show_hidden = false;
		sort = { null, false };
		list_attributes = FOLDER_ATTRIBUTES;
		load_job = null;
		expand_parents = new Queue<File>();
		current_folder_as_root = false;

		tree_model = new Gtk.TreeListModel (visible_roots.model, false, false, (obj) => {
			var file_data = obj as Gth.FileData;
			if (file_data != null) {
				return file_data.get_children_model ().model;
			}
			return null;
		});

		tree_selection_model = new Gtk.SingleSelection (tree_model);
		tree_selection_model.autoselect = false;
		tree_selection_model.can_unselect = true;
		tree_selection_model.notify["selected"].connect ((obj, _param) => {
			if (context_menu_visible) {
				return;
			}
			if (building_the_tree ()) {
				// Ignore selection changes when still building the tree.
				return;
			}
			var local_model = obj as Gtk.SingleSelection;
			var row = tree_selection_model.selected_item as Gtk.TreeListRow;
			if (row != null) {
				var file_data = row.item as Gth.FileData;
				if (!FileData.equal (file_data, current_folder)) {
					load_subfolder (file_data);
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
		scrolled_window.vadjustment.changed.connect (() => {
			if (scroll_to_selected) {
				queue_select_current_folder (false);
			}
		});
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
					load (file_data, LoadAction.OPEN_AS_ROOT);
				}
			}
		});
		scrolled_window.set_child (view);

		context_menu = new Gtk.PopoverMenu.from_model (menu_model);
		context_menu.has_arrow = true;
		context_menu.position = Gtk.PositionType.BOTTOM;
		context_menu.add_css_class ("deeper");
		context_menu.closed.connect (() => {
			if (selected_before_context_menu != -1) {
				tree_selection_model.set_selected (selected_before_context_menu);
				selected_before_context_menu = -1;
			}
			context_menu_visible = false;
		});
		append (context_menu);

		context_file = null;

		watched_files = new FileSet ();
		watched_files.added.connect ((file) => {
			var source = app.get_source_for_file (file);
			source.monitor_directory (file, true);
		});
		watched_files.removed.connect ((file) => {
			var source = app.get_source_for_file (file);
			source.monitor_directory (file, false);
		});
	}

	public void release_resources () {
		if (select_id != 0) {
			Source.remove (select_id);
			select_id = 0;
		}
		watched_files.remove_all ();
	}

	public void set_root (FileData root) {
		roots = new GenericList<FileData> ();
		roots.model.append (root);
	}

	~FolderTree () {
		release_resources ();
	}

	int selected_before_context_menu = -1;
	bool context_menu_visible;
	FileSet watched_files;
	GenericList<FileData> roots;
}
