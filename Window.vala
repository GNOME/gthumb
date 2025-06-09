[GtkTemplate (ui = "/app/gthumb/gthumb/ui/window.ui")]
public class Gth.Window : Adw.ApplicationWindow {
	public FileData current_root = null;
	public FileData current_folder = null;
	public GenericList<FileData> current_children = null;
	public GenericList<FileData> visible_files = null;
	public GenericList<FileData> root_children = null;
	public GenericList<FileData> roots = null;
	public FileSource folder_source = null;
	public string sort_name = null;
	public bool inverse_order = false;
	public bool fast_file_type = false;
	public bool show_hidden_files = false;
	public bool show_hidden_folders = false;
	public bool sidebar_visible = false;
	public bool sidebar_pinned = false;
	public int thumbnail_size;
	public Gth.JobQueue jobs;
	public string folder_sort_name = null;
	public bool folder_inverse_order = false;

	Queue<File> current_parents;

	public enum Page {
		NONE = 0,
		BROWSER,
		VIEWER,
	}

	public Window (Gtk.Application _app, File location, File? file_to_select) {
		Object (application: app);

		named_dialogs = new HashTable<string, Gtk.Window?>(str_hash, str_equal);
		jobs = new Gth.JobQueue ();
		visible_files = new GenericList<FileData>();
		root_children = new GenericList<FileData>();
		roots = new GenericList<FileData>();
		thumbnailer = new Thumbnailer ();
		history = new WindowHistory (this);
		bookmarks = new WindowBookmarks (this);
		action_group = new SimpleActionGroup ();
		insert_action_group ("win", action_group);
		current_parents = null;

		init_folder_tree ();
		init_file_grid ();
		init_actions ();

		filter_bar.changed.connect (() => update_active_filter ());

		browser_view.notify["show-sidebar"].connect (() => {
			sidebar_visible = browser_view.show_sidebar;
			action_group.change_action_state ("show-sidebar", new Variant.boolean (sidebar_visible));
			show_sidebar_button.visible = !sidebar_visible;
		});

		// Restore the window size.
		var width = app.browser_settings.get_int (PREF_BROWSER_WINDOW_WIDTH);
		var height = app.browser_settings.get_int (PREF_BROWSER_WINDOW_HEIGHT);
		set_default_size (width, height);
		if (app.browser_settings.get_boolean (PREF_BROWSER_WINDOW_MAXIMIZED)) {
			maximize ();
		}

		// Browser settings.
		sort_name = app.browser_settings.get_string (PREF_BROWSER_SORT_TYPE);
		inverse_order = app.browser_settings.get_boolean (PREF_BROWSER_SORT_INVERSE);
		general_filter = app.get_general_filter ();
		active_filter = app.get_last_active_filter ();
		fast_file_type = app.browser_settings.get_boolean (PREF_BROWSER_FAST_FILE_TYPE);
		show_hidden_files = app.browser_settings.get_boolean (PREF_BROWSER_SHOW_HIDDEN_FILES);
		thumbnail_size = app.browser_settings.get_int (PREF_BROWSER_THUMBNAIL_SIZE);
		folder_sort_name = app.browser_settings.get_string (PREF_BROWSER_FOLDER_TREE_SORT_TYPE);
		folder_inverse_order = app.browser_settings.get_boolean (PREF_BROWSER_FOLDER_TREE_SORT_INVERSE);

		set_page (Page.BROWSER);
		set_sidebar_pinned (true);

		// Load the location.
		title = "Thumbnails";
		Util.next_tick (() => {
			filter_bar.set_active_filter (active_filter);
			if (app.one_window ()) {
				history.restore_from_file ();
			}
			bookmarks.load_from_file.begin ((_obj, res) => {
				bookmarks.load_from_file.end (res);
				open_location (location, LoadAction.OPEN, file_to_select);
			});
		});
	}

	void set_file_data_children (FileData file_data, GenericList<FileData> all_children) {
		// Filter the folders
		var folders = new GenericList<FileData> ();
		foreach (unowned var file in all_children) {
			if (file.info.get_file_type () == FileType.DIRECTORY) {
				if (show_hidden_folders || !file.info.get_is_hidden ()) {
					folders.model.append (file);
				}
			}
		}

		// Sort the folders
		unowned var sort_info = app.get_folder_sorter_by_id (folder_sort_name);
		folders.model.sort ((a, b) => {
			var result = sort_info.cmp_func ((FileData) a, (FileData) b);
			if (folder_inverse_order)
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
		if (current_parents != null) {
			var top_parent = current_parents.peek_head ();
			//stdout.printf ("  top_parent: %s\n", (top_parent != null) ? top_parent.get_uri () : "(null)");
			if ((top_parent != null) && top_parent.equal (file_data.file)) {
				current_parents.pop_head ();
				var next_parent = current_parents.peek_head ();
				if (next_parent == null) {
					select_current_folder ();
				}
				else if ((current_folder != null) && (current_folder.file.equal (next_parent))) {
					current_parents.pop_head ();
					select_current_folder ();
				}
				else {
					//stdout.printf ("  next_parent: '%s'\n", next_parent.get_uri ());
					var next_parent_row = find_file_row_in_tree (next_parent);
					if (next_parent_row != null) {
						next_parent_row.expanded = true;
					}
				}
			}
		}
	}

	void select_current_folder () {
		if (current_folder == null)
			return;
		int position;
		var current_row = find_file_row_in_tree (current_folder.file, out position);
		//stdout.printf ("CURRENT FOLDER POS: %d\n", position);
		if (position >= 0) {
			folder_tree.scroll_to ((uint) position, Gtk.ListScrollFlags.SELECT, null);
		}
	}

	public Gth.Job? list_subfolders (FileData file_data) {
		//stdout.printf ("> LIST SUBFOLDERS %s\n", file_data.file.get_uri ());

		if (FileData.equal (file_data, current_folder)) {
			set_file_data_children (file_data, current_children);
			return null;
		}

		var source = app.get_source_for_file (file_data.file);
		if (source == null) {
			throw new IOError.FAILED (_("File type not supported"));
		}
		var local_job = new_job ("List subfolders for %s".printf (file_data.file.get_uri ()));
		source.list_children.begin (file_data.file, STANDARD_ATTRIBUTES, local_job.cancellable, (_obj, res) => {
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

	Gth.Job load_job = null;

	async void load_folder (File location, LoadAction load_action) throws Error {
		var source = app.get_source_for_file (location);
		if (source == null) {
			throw new IOError.FAILED (_("File type not supported"));
		}
		if (load_job != null) {
			load_job.cancel ();
		}
		thumbnailer.cancel ();
		if (load_action.changes_root ()) {
			if (current_parents != null) {
				current_parents.clear ();
			}
		}

		var local_job = new_job ("Load folder %s".printf (location.get_uri ()));
		load_job = local_job;
		try {
			// Load the requested location.
			var file_data = yield source.read_metadata (location, "*", local_job.cancellable);
			var children = yield source.list_children (location, get_list_attributes (true), local_job.cancellable);

			var location_is_root = false;
			if (load_action.changes_root ()) {
				var nearest_root = (load_action == LoadAction.OPEN_AS_ROOT) ? file_data : Util.get_nearest_parent (location, bookmarks.roots);
				//stdout.printf (">> nearest_root: %s\n", (nearest_root != null) ? nearest_root.file.get_uri () : "(nil)");

				current_parents = new Queue<File>();
				var parent = location;
				while (parent != null) {
					//stdout.printf (">> add parent: %s\n", parent.get_uri ());
					current_parents.push_head (parent);
					if (parent.equal (nearest_root.file)) {
						break;
					}
					parent = parent.get_parent ();
				}
				if (location.equal (nearest_root.file)) {
					current_root = file_data;
					location_is_root = true;
				}
				else {
					var root = current_parents.peek_head ();
					current_root = yield source.read_metadata (root, "*", local_job.cancellable);
				}
			}

			if (load_action.changes_current_folder ()) {
				folder_source = source;
				current_folder = file_data;
				current_children = children;
				update_thumbnail_list ();
				update_title ();
				// TODO source.monitor_directory (current_folder.file, true);
			}

			if (load_action != LoadAction.OPEN_FROM_HISTORY) {
				history.add (current_folder.file);
			}

			if (load_action.changes_root ()) {
				update_folder_tree ();
				if (current_root.file.get_uri ().has_prefix ("catalog:")) {
					catalog_button.active = true;
					vfs_button.active = false;
				}
				else {
					catalog_button.active = false;
					vfs_button.active = true;
				}
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

	public bool is_loading () {
		return (load_job != null) && load_job.is_running ();
	}

	public void show_message (string message) {
		// TODO
	}

	public void open_location (File location, LoadAction load_action = LoadAction.OPEN, File? file_to_select = null) {
		set_page (Page.BROWSER);
		load_folder.begin (location, load_action, (_obj, res) => {
			try {
				load_folder.end (res);
				if (file_to_select != null) {
					// TODO
				}
			}
			catch (Error error) {
				show_message (error.message);
			}
		});
	}

	public void load_subfolder (File location) {
		load_folder.begin (location, LoadAction.OPEN_SUBFOLDER, (_obj, res) => {
			try {
				load_folder.end (res);
				// TODO
			}
			catch (Error error) {
				show_message (error.message);
			}
		});
	}

	public void update_sort_order (string _sort_name, bool _inverse_order) {
		sort_name = _sort_name;
		inverse_order = _inverse_order;
		update_thumbnail_list ();
	}

	public void set_page (Page page) {
		if (page == current_page)
			return;
		current_page = page;
		/*switch (current_page) {
		case Page.BROWSER:
			main_stack.set_visible_child (browser_page);
			break;
		case Page.VIEWER:
			main_stack.set_visible_child (viewer_page);
			break;
		}*/
		update_title ();
		update_sensitivity ();
	}

	public void set_named_dialog (string name, Gtk.Window dialog) {
		dialog.close_request.connect (() => {
			named_dialogs[name] = null;
			return false;
		});
		named_dialogs[name] = dialog;
	}

	public Gtk.Window get_named_dialog (string name) {
		return named_dialogs[name];
	}

	public Gth.Job new_job (string description, string? status = null) {
		var job = app.new_job (description, status);
		add_job (job);
		return job;
	}

	public Gth.Job new_background_job (string description, string? status = null) {
		var job = app.new_job (description, status, true);
		add_job (job);
		return job;
	}

	public void add_job (Gth.Job job) {
		jobs.add_job (job);
	}

	public void cancel_jobs () {
		jobs.cancel_all ();
	}

	void update_title () {
		if (current_folder != null) {
			title = current_folder.info.get_display_name ();
		}
		else {
			title = "Thumbnails";
		}
	}

	void update_sensitivity () {
		// TODO
	}

	string list_attributes = null;
	string[] thumbnail_attributes_v = {};

	unowned string get_list_attributes (bool recalc) {
		if (recalc) {
			list_attributes = null;
		}
		if (list_attributes != null) {
			return list_attributes;
		}

		var attributes = new StringBuilder ();

		// Standard attributes.
		if (fast_file_type) {
			attributes.append (STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE);
		}
		else {
			attributes.append (STANDARD_ATTRIBUTES_WITH_CONTENT_TYPE);
		}

		// Attributes required by the filter.
		unowned var filter = get_file_filter ();
		if (filter.has_attributes ()) {
			attributes.append (",");
			attributes.append (filter.attributes);
		}

		// Attributes required for sorting.
		if (sort_name != null) {
			var sort_info = app.get_sorter_by_id (sort_name);
			if (sort_info != null) {
				if (!Strings.empty (sort_info.required_attributes)) {
					attributes.append (",");
					attributes.append (sort_info.required_attributes);
				}
			}
		}

		// Attributes required for the thumbnail caption.
		var thumbnail_caption = app.browser_settings.get_string (PREF_BROWSER_THUMBNAIL_CAPTION);
		if (!Strings.empty (thumbnail_caption) && (thumbnail_caption != "none")) {
			attributes.append (",");
			attributes.append (thumbnail_caption);
			thumbnail_attributes_v = thumbnail_caption.split (",");
		}
		else {
			thumbnail_attributes_v = {};
		}

		// Other attributes. TODO
		//attributes.append (",");
		//attributes.append (GTH_ATTRIBUTE_EMBLEMS);

		// Save in a variable to avoid recalculation.
		list_attributes = attributes.str;
		return list_attributes;
	}

	Gth.Filter? file_filter = null;

	public unowned Gth.Filter get_file_filter (bool recalc = false) {
		if ((file_filter == null) || recalc) {
			file_filter = app.add_general_filter (active_filter);
			if (!show_hidden_files) {
				file_filter.tests.add (new Gth.TestVisible ());
			}
		}
		return file_filter;
	}

	void update_active_filter () {
		active_filter = filter_bar.filter.duplicate ();
		update_thumbnail_list ();
	}

	void update_folder_tree () {
		roots.model.remove_all ();
		root_children.model.remove_all ();
		foreach (unowned var file in current_children) {
			if (file.info.get_file_type () == FileType.DIRECTORY) {
				if (show_hidden_folders || !file.info.get_is_hidden ()) {
					root_children.model.append (file);
				}
			}
		}
		roots.model.append (current_root);

		var root_row = find_file_row_in_tree (current_root.file);
		if (root_row != null) {
			root_row.expanded = true;
		}
	}

	void update_thumbnail_list () {
		// Filter the files.
		var visible_children = new GenericList<FileData>();
		var filter = get_file_filter (true);
		var iter = filter.iterator (current_children);
		while (iter.next ()) {
			visible_children.model.append (iter.get ());
		}

		// Sort the files.
		if ((sort_name != filter.sort_name) || (inverse_order != filter.inverse_order)) {
			unowned var sort_info = app.get_sorter_by_id (sort_name);
			if (sort_info == null) {
				sort_info = app.get_sorter_by_id ("file::name");
			}
			if (sort_info.cmp_func != null) {
				visible_children.model.sort ((a, b) => {
					var result = sort_info.cmp_func ((FileData) a, (FileData) b);
					if (inverse_order)
						result *= -1;
					return result;
				});
			}
		}

		// Update the view model.
		visible_files.model.remove_all ();
		var tot_files = 0;
		uint64 tot_size = 0;
		foreach (unowned var file in visible_children) {
			visible_files.model.append (file);
			tot_files++;
			tot_size += file.info.get_size ();
		}

		status.set_list_info (tot_files, tot_size);
		folder_stack.set_visible_child ((tot_files == 0) ? empty_folder : non_empty_folder);
	}

	void update_selection_info () {
		var tot_files = 0;
		uint64 tot_size = 0;
		var selected = file_grid.model.get_selection ();
		for (int64 idx = 0; idx < selected.get_size (); idx++) {
			var pos = selected.get_nth ((uint) idx);
			var file = file_grid.model.get_item (pos) as FileData;
			if (file != null) {
				tot_files++;
				tot_size += file.info.get_size ();
			}
		}
		status.set_selection_info (tot_files, tot_size);
	}

	void init_actions () {
		var builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/app-menu.ui");
		app_menu_button.menu_model = builder.get_object ("app_menu") as MenuModel;

		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/bookmarks-menu.ui");
		bookmarks_button.menu_model = builder.get_object ("bookmarks_menu") as MenuModel;
		bookmarks.menu = builder.get_object ("app-bookmarks") as Menu;
		bookmarks.system_menu = builder.get_object ("system-bookmarks") as Menu;
		bookmarks.roots_menu = builder.get_object ("roots") as Menu;

		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/history-menu.ui");
		history_button.menu_model = builder.get_object ("history_menu") as MenuModel;
		history.menu = builder.get_object ("history_entries") as Menu;

		var action = new SimpleAction ("about", null);
		action.activate.connect (() => {
			const string[] developers = {
				"Paolo Bacchilega <paobac@src.gnome.org>",
			};
			Adw.show_about_dialog (this,
				"application-name", "Thumbnails",
				"application-icon", "app.gthumb.gthumb",
				"version", Config.VERSION,
				"license-type", Gtk.License.GPL_2_0,
				"translator-credits", _("translator-credits"),
				"website", "https://gitlab.gnome.org/GNOME/gthumb/",
				"issue-url", "https://gitlab.gnome.org/GNOME/gthumb/-/issues",
				"developers", developers
			);
		});
		action_group.add_action (action);

		action = new SimpleAction ("edit-filters", null);
		action.activate.connect (() => {
			var dialog = new Gth.EditFiltersDialog ();
			dialog.present (this);
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("set-general-filter", VariantType.STRING, new Variant.string ((general_filter != null) ? general_filter.id : ""));
		action.activate.connect ((_action, param) => {
			_action.set_state (param);
			// TODO
		});
		action_group.add_action (action);

		action = new SimpleAction ("sort-files", null);
		action.activate.connect (() => {
			var dialog = get_named_dialog ("sort-files");
			if (dialog == null) {
				dialog = new Gth.SortFilesDialog (this);
				set_named_dialog ("sort-files", dialog);
			}
			dialog.present ();
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("set-sort-name", GLib.VariantType.STRING, new Variant.string (sort_name ?? ""));
		action.activate.connect ((_action, param) => {
			_action.set_state (param);
			// TODO
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("set-inverse-order", GLib.VariantType.BOOLEAN, new Variant.boolean (inverse_order));
		action.activate.connect ((_action, param) => {
			_action.set_state (param);
			// TODO
		});
		action_group.add_action (action);

		action = new SimpleAction ("new-window", null);
		action.activate.connect (() => {
			var window = new Gth.Window (app, current_folder.file, null);
			window.present ();
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("show-hidden-files", null, new Variant.boolean (show_hidden_files));
		action.activate.connect ((_action, param) => {
			show_hidden_files = Util.toggle_state (_action);
			update_thumbnail_list ();
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("show-sidebar", null, new Variant.boolean (sidebar_visible));
		action.activate.connect ((_action, param) => {
			sidebar_visible = Util.toggle_state (_action);
			browser_view.show_sidebar = sidebar_visible;
			show_sidebar_button.visible = !sidebar_visible;
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("pin-sidebar", null, new Variant.boolean (sidebar_pinned));
		action.activate.connect ((_action, param) => {
			set_sidebar_pinned (Util.toggle_state (_action));
		});
		action_group.add_action (action);

		action = new SimpleAction ("load-home", null);
		action.activate.connect ((_action, param) => {
			File home = null;
			if (app.browser_settings.get_boolean (PREF_BROWSER_USE_STARTUP_LOCATION)) {
				var uri = app.browser_settings.get_string (PREF_BROWSER_STARTUP_LOCATION);
				var full_uri = Files.expand_home_uri (uri);
				home = File.new_for_uri (full_uri);
			}
			open_location ((home != null) ? home : Files.get_home ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("load-catalog-home", null);
		action.activate.connect ((_action, param) => {
			open_location (File.new_for_uri ("catalog:///"));
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("load-history-position", VariantType.INT16, new Variant.int16 ((int16) history.current));
		action.activate.connect ((_action, param) => {
			history.load (param.get_int16 ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("delete-history", null);
		action.activate.connect ((_action, param) => history.clear ());
		action_group.add_action (action);

		action = new SimpleAction ("load-previous", null);
		action.activate.connect ((_action, param) => {
			history.load_previous ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("load-next", null);
		action.activate.connect ((_action, param) => {
			history.load_next ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("load-location", VariantType.STRING);
		action.activate.connect ((_action, param) => {
			open_location (File.new_for_uri (param.get_string ()));
		});
		action_group.add_action (action);
	}

	Gtk.TreeListModel tree_model = null;
	Gtk.SingleSelection tree_selection_model = null;

	void init_folder_tree () {
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
			if ((current_parents != null) && !current_parents.is_empty ()) {
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
		folder_tree.model = tree_selection_model;

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
		folder_tree.factory = factory;

		folder_tree.activate.connect ((position) => {
			var row = tree_model.get_row (position);
			if (row != null) {
				var file_data = row.item as Gth.FileData;
				if (file_data != null) {
					load_folder (file_data.file, LoadAction.OPEN_AS_ROOT);
				}
			}
		});
	}

	void init_file_grid () {
		file_grid.model = new Gtk.MultiSelection (visible_files.model);
		file_grid.model.selection_changed.connect (() => {
			update_selection_info ();
		});

		var factory = new Gtk.SignalListItemFactory ();
		factory.setup.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			list_item.child = new Gth.FileListItem (thumbnail_size, thumbnail_attributes_v);
		});
		factory.teardown.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			list_item.child = null;
		});
		factory.bind.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			var file_item = list_item.child as Gth.FileListItem;
			if (file_item != null) {
				var file_data = list_item.item as FileData;
				if (file_data != null) {
					file_item.bind (file_data);
					thumbnailer.requested_size = (uint) thumbnail_size;
					thumbnailer.add (file_data);
				}
			}
		});
		factory.unbind.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			var file_item = list_item.child as Gth.FileListItem;
			if (file_item != null) {
				file_item.unbind ();
				var file_data = list_item.item as FileData;
				if (file_data != null) {
					thumbnailer.remove (file_data);
					file_data.set_thumbnail (null);
				}
			}
		});
		file_grid.factory = factory;
	}

	void set_sidebar_pinned (bool value) {
		sidebar_pinned = value;
		browser_view.collapsed = !sidebar_pinned;
		if (browser_view.collapsed) {
			browser_view.show_sidebar = true;
		}
		show_sidebar_button.visible = !sidebar_visible;
		action_group.change_action_state ("pin-sidebar", new Variant.boolean (sidebar_pinned));
	}

	Gtk.TreeListRow? find_file_row_in_tree (File file, out int position = null) {
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

	Gth.FileData? find_folder_in_tree (File file) {
		var row = find_file_row_in_tree (file);
		return (row != null) ? row.item as Gth.FileData : null;
	}

	[GtkChild] unowned Adw.OverlaySplitView browser_view;
	[GtkChild] unowned Gth.FilterBar filter_bar;
	[GtkChild] unowned Gtk.MenuButton app_menu_button;
	[GtkChild] unowned Gtk.MenuButton bookmarks_button;
	[GtkChild] unowned Gtk.MenuButton history_button;
	[GtkChild] unowned Gtk.GridView file_grid;
	[GtkChild] unowned Gtk.ListView folder_tree;
	[GtkChild] public unowned Gth.Status status;
	[GtkChild] unowned Gtk.Stack folder_stack;
	[GtkChild] unowned Gtk.Widget non_empty_folder;
	[GtkChild] unowned Gtk.Widget empty_folder;
	[GtkChild] unowned Gtk.Widget show_sidebar_button;
	[GtkChild] unowned Gtk.ToggleButton vfs_button;
	[GtkChild] unowned Gtk.ToggleButton catalog_button;

	Page current_page = Page.NONE;
	public SimpleActionGroup action_group = null;
	Gth.Test general_filter;
	Gth.Test active_filter;
	HashTable<string, Gtk.Window?> named_dialogs;
	Gth.Thumbnailer thumbnailer;
	Gth.WindowHistory history;
	Gth.WindowBookmarks bookmarks;
}
