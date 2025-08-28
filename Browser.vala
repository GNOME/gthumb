[GtkTemplate (ui = "/app/gthumb/gthumb/ui/browser.ui")]
public class Gth.Browser : Gtk.Box {
	public weak Window window {
		get { return _window; }
		set {
			_window = value;
			init ();
		}
	}

	enum SidebarState {
		NONE,
		FILES,
		CATALOGS,
	}

	public IterableList<FileData> visible_files;
	public bool fast_file_type = false;
	public bool show_hidden_files = false;
	SidebarState sidebar_state = SidebarState.NONE;
	public bool open_in_fullscreen = false;
	public int thumbnail_size;
	[GtkChild] public unowned Gth.FolderTree folder_tree;
	[GtkChild] public unowned Gth.Status status;
	public bool never_loaded;
	public Gth.FileSorter file_sorter;
	public Gth.FileFilter file_filter;

	construct {
		history = new History (this);
		current_parents = null;
		binded_grid_items = new GenericArray<Gtk.ListItem> ();
		actions_category = new ActionCategory ("", -1);
		bookmarks_category = new ActionCategory (_("Bookmarks"), 1);
		parents_category = new ActionCategory (_("Path"), 1);
		never_loaded = true;
	}

	void init () {
		thumbnailer = new Thumbnailer.for_browser (this);

		init_folder_tree ();
		init_file_grid ();

		sidebar_resizer.add_handle (main_view, Gtk.PackType.END);
		sidebar_resizer.started.connect ((obj) => {
			window.active_resizer = obj;
		});
		sidebar_resizer.ended.connect (() => {
			window.active_resizer = null;
		});

		property_sidebar.resizer.add_handle (content_view, Gtk.PackType.START);
		property_sidebar.resizer.started.connect ((obj) => {
			window.active_resizer = obj;
		});
		property_sidebar.resizer.ended.connect (() => {
			window.active_resizer = null;
		});

		filter_bar.changed.connect (() => update_active_filter ());

		main_view.notify["show-sidebar"].connect (() => {
			window.action_group.change_action_state ("show-browser-sidebar", new Variant.boolean (main_view.show_sidebar));
		});

		// Restore the window size.
		browser_width = app.settings.get_int (PREF_BROWSER_WINDOW_WIDTH);
		browser_height = app.settings.get_int (PREF_BROWSER_WINDOW_HEIGHT);
		window.set_default_size (browser_width, browser_height);
		if (app.settings.get_boolean (PREF_BROWSER_WINDOW_MAXIMIZED)) {
			window.maximize ();
		}
		main_view.max_sidebar_width = (double) app.settings.get_int (PREF_BROWSER_SIDEBAR_WIDTH);
		content_view.max_sidebar_width = (double) app.settings.get_int (PREF_BROWSER_PROPERTIES_WIDTH);

		// Restore settings.

		file_sorter.set_order (
			app.migration.test.get_new_key (app.settings.get_string (PREF_BROWSER_SORT_TYPE)),
			app.settings.get_boolean (PREF_BROWSER_SORT_INVERSE));

		general_filter = app.get_general_filter ();
		show_hidden_files = app.settings.get_boolean (PREF_BROWSER_SHOW_HIDDEN_FILES);
		file_filter.update_filter ();

		fast_file_type = app.settings.get_boolean (PREF_BROWSER_FAST_FILE_TYPE);
		thumbnail_size = app.settings.get_int (PREF_BROWSER_THUMBNAIL_SIZE);
		folder_tree.sort = {
			app.settings.get_string (PREF_BROWSER_FOLDER_TREE_SORT_TYPE),
			app.settings.get_boolean (PREF_BROWSER_FOLDER_TREE_SORT_INVERSE)
		};
		folder_tree.show_hidden = show_hidden_files;
		content_view.show_sidebar = app.settings.get_boolean (PREF_BROWSER_PROPERTIES_VISIBLE);
		open_in_fullscreen = app.settings.get_boolean (PREF_BROWSER_OPEN_IN_FULLSCREEN);

		init_actions ();
	}

	async void add_files (GenericList<File> files) {
		freeze_thumbnail_list ();
		var attributes = get_list_attributes ();
		var local_job = window.new_job (_("Updating %s").printf (folder_tree.current_folder.get_display_name ()),
			JobFlags.FOREGROUND,
			"folder-symbolic");
		foreach (var file in files) {
			try {
				var info = yield Files.query_info (file, attributes, local_job.cancellable);
				var file_data = new FileData (file, info);
				folder_tree.current_children.model.append (file_data);
			}
			catch (Error error) {
				if (error is IOError.CANCELLED) {
					break;
				}
			}
		}
		local_job.done ();
		thaw_thumbnail_list ();
	}

	public async void load_folder (File location, LoadAction load_action, Job? job = null) throws Error {
		thumbnailer.cancel ();
		freeze_thumbnail_list ();
		folder_tree.list_attributes = get_list_attributes (true);
		yield folder_tree.load_folder (location, load_action, job);
		thaw_thumbnail_list ();
		update_title ();
		update_load_sensitivity ();
		update_location_commands ();
		var is_catalog = folder_tree.current_folder.file.has_uri_scheme ("catalog");
		set_sidebar_state (is_catalog ? SidebarState.CATALOGS : SidebarState.FILES);
		if (is_catalog) {
			last_catalog = folder_tree.current_folder.file;
		}
		else {
			last_folder = folder_tree.current_folder.file;
		}
		// TODO source.monitor_directory (current_folder.file, true);
		update_selection_info ();
		update_location_menu ();
		if (load_action != LoadAction.OPEN_FROM_HISTORY) {
			history.add (folder_tree.current_folder.file);
		}
		thumbnailer.requested_size = (uint) thumbnail_size;
		thumbnailer.queue_load_next ();
	}

	public async void open_location_async (File location, LoadAction load_action = LoadAction.OPEN, Job? job = null) throws Error {
		yield window.set_page (Window.Page.BROWSER);
		yield load_folder (location, load_action, job);
	}

	public void open_location (File location, LoadAction load_action = LoadAction.OPEN, File? file_to_select = null) {
		open_location_async.begin (location, load_action, null, (_obj, res) => {
			try {
				open_location_async.end (res);
				if (file_to_select != null) {
					select_file (file_to_select, SelectFile.SCROLL_TO_FILE);
				}
			}
			catch (Error error) {
				window.show_error (error);
			}
		});
	}

	public void open_home () {
		File home = null;
		if (app.settings.get_boolean (PREF_BROWSER_USE_STARTUP_LOCATION)) {
			home = Gth.Settings.get_startup_location (app.settings);
		}
		if (home == null) {
			home = Files.get_home ();
		}
		open_location (home);
	}

	public async void first_load () {
		never_loaded = false;
		if (app.one_window ()) {
			// Restore the saved settings.
			history.restore_from_file ();
			filter_bar.restore_from_file ();
		}
		else {
			// Copy the settings from the previously focused window.
			unowned var windows = app.get_windows ();
			foreach (unowned var win in windows) {
				if ((win != window) && (win is Gth.Window)) {
					var previous_window = win as Gth.Window;
					history.copy (previous_window.browser.history);
					filter_bar.set_active_filter (previous_window.browser.filter_bar.filter);
					break;
				}
			}
		}
		yield app.bookmarks.load_from_file ();
		update_bookmarks_menu ();
	}

	public void reload () {
		if (folder_tree.current_folder == null) {
			return;
		}
		open_location (folder_tree.current_folder.file, LoadAction.OPEN_SUBFOLDER);
	}

	public void set_file_order (string name, bool inverse) {
		file_sorter.set_order (name, inverse);
	}

	public void set_file_inverse_order (bool inverse) {
		file_sorter.set_inverse (inverse);
	}

	void set_sidebar_state (SidebarState state) {
		if (state == sidebar_state) {
			return;
		}
		sidebar_state = state;
		switch (sidebar_state) {
		case SidebarState.FILES:
			sidebar_stack.set_visible_child (folder_tree);
			vfs_button.active = true;
			catalog_button.active = false;
			break;
		case SidebarState.CATALOGS:
			sidebar_stack.set_visible_child (folder_tree);
			vfs_button.active = false;
			catalog_button.active = true;
			break;
		case SidebarState.NONE:
			break;
		}
	}

	void set_show_hidden (bool show) {
		show_hidden_files = show;
		folder_tree.show_hidden = show_hidden_files;
		file_filter.update_filter ();
		update_total_files ();
	}

	public void open_file_context_menu (Gth.FileListItem item, int x, int y) {
		if (!is_selected (item.file_data.file)) {
			select_file (item.file_data.file);
		}
		Graphene.Point p = Graphene.Point.zero ();
		item.compute_point (non_empty_folder, Graphene.Point.zero (), out p);
		file_context_menu.pointing_to = { (int) p.x + x, (int) p.y + y, 1, 12 };
		file_context_menu.popup ();
	}

	public void open_context_menu (int x, int y) {
		Graphene.Point p = Graphene.Point.zero ();
		file_grid.compute_point (folder_stack.get_visible_child (), Graphene.Point.zero (), out p);
		context_menu.pointing_to = { (int) p.x + x, (int) p.y + y, 1, 12 };
		context_menu.popup ();
	}

	void update_load_sensitivity () {
		File parent = null;
		if (folder_tree.current_folder != null) {
			parent = folder_tree.current_folder.file.get_parent ();
		}
		Util.enable_action (window.action_group, "load-parent", parent != null);
	}

	void update_location_commands () {
		var is_catalog = false;
		var is_search = false;
		var uri = folder_tree.current_folder.file.get_uri ();
		var source_type = app.get_source_type_for_uri (uri);
		if (folder_tree.current_folder.file.has_uri_scheme ("catalog")) {
			is_catalog = uri.has_suffix (".catalog");
			is_search = uri.has_suffix (".search");
		}
		edit_catalog_button.visible = is_catalog || is_search;
		update_search_button.visible = is_search;
		update_folder_button.visible = (source_type == typeof (FileSourceVfs));
	}

	string list_attributes = null;
	string[] thumbnail_attributes_v = {};

	public unowned string get_list_attributes (bool recalc = false) {
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
		if (file_filter.filter != null) {
			if (file_filter.filter.has_attributes ()) {
				attributes.append (",");
				attributes.append (file_filter.filter.attributes);
			}
		}

		// Attributes required for sorting.
		if (file_sorter.sort_info != null) {
			if (!Strings.empty (file_sorter.sort_info.required_attributes)) {
				attributes.append (",");
				attributes.append (file_sorter.sort_info.required_attributes);
			}
		}

		// Attributes required for the thumbnail caption.
		var thumbnail_caption = app.settings.get_string (PREF_BROWSER_THUMBNAIL_CAPTION);
		if (!Strings.empty (thumbnail_caption) && (thumbnail_caption != "none")) {
			thumbnail_attributes_v = app.migration.metadata.get_new_key_v (thumbnail_caption);
			attributes.append (",");
			attributes.append (string.joinv (",", thumbnail_attributes_v));
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

	void update_active_filter () {
		file_filter.update_filter ();
		update_total_files ();
	}

	public void update_title () {
		string title;
		if (folder_tree.current_folder != null) {
			title = folder_tree.current_folder.info.get_display_name ();
		}
		else {
			title = "Thumbnails";
		}
		if (window.current_page == Window.Page.BROWSER) {
			window.title = title;
		}
		location_button.label = title;
	}

	Gtk.SelectionModel grid_model = null;
	ListModel sorted_model = null;
	ListModel filtered_model = null;

	void freeze_thumbnail_list () {
		grid_model = file_grid.model;
		sorted_model = file_sort_model.model;
		filtered_model = file_filter_model.model;
		file_grid.model = null;
		file_sort_model.model = null;
		file_filter_model.model = null;
	}

	void update_total_files () {
		total_files = 0;
		total_size = 0;
		foreach (unowned var file in visible_files) {
			total_files++;
			total_size += file.info.get_size ();
		}
		status.set_list_info (total_files, total_size);
		window.viewer.set_list_info (total_files);
		update_folder_status ();
	}

	void thaw_thumbnail_list () {
		file_filter.reset ();
		file_filter_model.model = filtered_model;
		file_sort_model.model = sorted_model;
		file_grid.model = grid_model;
		update_total_files ();
	}

	public void update_folder_status () {
		folder_stack.set_visible_child ((total_files == 0) ? empty_folder : non_empty_folder);
		if (total_files == 0) {
			var title = _("No Files");
			if ((filter_bar.filter != null) && (filter_bar.filter.id != "")) {
				title = _("No Matches");
				folder_status.icon_name = "gth-filter-symbolic";
			}
			else {
				folder_status.gicon = folder_tree.current_folder.get_symbolic_icon ();
				switch (folder_tree.current_folder.get_content_type ()) {
				case "gthumb/search":
					title = _("No Files Found");
					break;
				case "gthumb/catalog":
					title = _("Empty Catalog");
					break;
				case "gthumb/library":
					title = _("Library");
					break;
				default:
					title = _("Empty Folder");
					break;
				}
			}
			folder_status.title = title;
		}
	}

	void update_location_menu () {
		location_actions.remove_all_actions ();
		var location = folder_tree.current_folder.file;
		while (location != null) {
			var action = new ActionInfo.for_file ("win.load-location", location);
			action.category = parents_category;
			location_actions.append_action (action);
			location = location.get_parent ();
		}
	}

	public void update_bookmarks_menu () {
		var bookmark_menu = bookmark_popover.actions;
		bookmark_menu.remove_all_actions ();

		var action = new ActionInfo ("win.add-bookmark", null, _("_Add To Bookmarks"));
		action.category = actions_category;
		bookmark_menu.append_action (action);

		action = new ActionInfo ("win.edit-bookmarks", null, _("_Edit Bookmarks…"));
		action.category = actions_category;
		bookmark_menu.append_action (action);

		// Roots
		foreach (unowned var file_action in app.bookmarks.roots) {
			bookmark_menu.append_action (file_action);
		}

		// Bookmarks
		foreach (unowned var entry in app.bookmarks.entries) {
			action = new ActionInfo.for_file ("win.load-location", entry.file, entry.display_name);
			action.category = bookmarks_category;
			bookmark_menu.append_action (action);
		}

		// System bookmarks
		foreach (unowned var file_action in app.bookmarks.system_bookmarks) {
			bookmark_menu.append_action (file_action);
		}
	}

	public uint get_selected_position () {
		var selected = file_grid.model.get_selection ();
		if (selected.get_size () != 1) {
			// TODO: the last context menu item.
			return uint.MAX;
		}
		return selected.get_nth (0);
	}

	public FileData? get_selected_file_data () {
		var pos = get_selected_position ();
		if (pos == uint.MAX) {
			return null;
		}
		var file_data = file_grid.model.get_item (pos) as FileData;
		return file_data;
	}

	public File? get_selected_file () {
		var file_data = get_selected_file_data ();
		return (file_data != null) ? file_data.file : null;
	}

	public GenericList<File> get_selected_files () {
		var files = new GenericList<File> ();
		var selection = file_grid.model.get_selection ();
		var n_selected = selection.get_size ();
		for (var i = 0; i < n_selected; i++) {
			var pos = selection.get_nth (i);
			var selected_file = file_grid.model.get_item (pos) as FileData;
			files.model.append (selected_file.file);
		}
		return files;
	}

	public GenericList<FileData> get_selected_file_data_list () {
		var files = new GenericList<FileData> ();
		var selection = file_grid.model.get_selection ();
		var n_selected = selection.get_size ();
		for (var i = 0; i < n_selected; i++) {
			var pos = selection.get_nth (i);
			var selected_file = file_grid.model.get_item (pos) as FileData;
			files.model.append (selected_file);
		}
		return files;
	}

	bool is_selected (File file) {
		var selection = file_grid.model.get_selection ();
		for (int64 idx = 0; idx < selection.get_size (); idx++) {
			var pos = selection.get_nth ((uint) idx);
			var selected_file = file_grid.model.get_item (pos) as FileData;
			if (selected_file.file.equal (file)) {
				return true;
			}
		}
		return false;
	}

	public enum SelectFile {
		DEFAULT,
		SCROLL_TO_FILE
	}

	public void select_file (File file, SelectFile flags = SelectFile.DEFAULT) {
		var iter = visible_files.iterator ();
		var pos = iter.find_first ((file_data) => file_data.file.equal (file));
		select_position (pos, flags);
	}

	public uint get_file_position (File file) {
		var iter = visible_files.iterator ();
		var pos = iter.find_first ((file_data) => file_data.file.equal (file));
		return (pos >= 0) ? (uint) pos : uint.MAX;
	}

	public void select_position (int position, SelectFile flags = SelectFile.DEFAULT) {
		if (position < 0)
			return;
		if (SelectFile.SCROLL_TO_FILE in flags) {
			file_grid.scroll_to ((uint) position, Gtk.ListScrollFlags.SELECT | Gtk.ListScrollFlags.FOCUS, null);
		}
		else {
			file_grid.model.select_item ((uint) position, true);
		}
	}

	public GenericArray<Gth.FileData> get_selected () {
		var selected_files = new GenericArray<Gth.FileData>();
		var selected = file_grid.model.get_selection ();
		for (int64 idx = 0; idx < selected.get_size (); idx++) {
			var pos = selected.get_nth ((uint) idx);
			var file = file_grid.model.get_item (pos) as FileData;
			if (file != null) {
				selected_files.add (file);
			}
		}
		return selected_files;
	}

	void update_selection_info () {
		weak Gth.FileData selected_file = null;
		uint total_files = 0;
		uint64 total_size = 0;
		var selected = file_grid.model.get_selection ();
		for (int64 idx = 0; idx < selected.get_size (); idx++) {
			var pos = selected.get_nth ((uint) idx);
			var file = file_grid.model.get_item (pos) as FileData;
			if (file != null) {
				total_files++;
				total_size += file.info.get_size ();
				selected_file = file;
			}
		}
		if (total_files == 1) {
			var local_job = window.new_job ("Metadata for %s".printf (selected_file.get_display_name ()),
				JobFlags.DEFAULT,
				"gth-note-symbolic");
			property_sidebar.load.begin (selected_file, local_job.cancellable, (_obj, res) => {
				try {
					property_sidebar.load.end (res);
				}
				catch (Error error) {
					stdout.printf ("ERROR: %s\n", error.message);
				}
				finally {
					local_job.done ();
				}
			});
		}
		else {
			//property_sidebar.current_file = null;
			property_sidebar.set_selection_info (total_files, total_size);
		}
	}

	void init_actions () {
		var builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/browser-menu.ui");
		app_menu_button.menu_model = builder.get_object ("app_menu") as MenuModel;

		history.actions = history_popover.actions;

		var action_group = window.action_group;

		var action = new SimpleAction ("edit-filters", null);
		action.activate.connect (() => {
			var dialog = new Gth.FiltersDialog ();
			dialog.present (this);
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("set-general-filter", VariantType.STRING, new Variant.string ((general_filter != null) ? general_filter.id : ""));
		action.activate.connect ((_action, param) => {
			_action.set_state (param);
			filter_bar.general_filter_changed ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("sort-files", null);
		action.activate.connect (() => {
			var dialog = new Gth.SortFilesDialog (this);
			dialog.present (this);
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("set-sort-name",
			GLib.VariantType.STRING,
			new Variant.string (file_sorter.name ?? ""));
		action.activate.connect ((_action, param) => {
			_action.set_state (param);
			// TODO
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("set-inverse-order",
			GLib.VariantType.BOOLEAN,
			new Variant.boolean (file_sorter.inverse));
		action.activate.connect ((_action, param) => {
			_action.set_state (param);
			// TODO
		});
		action_group.add_action (action);

		action = new SimpleAction ("sort-folders", null);
		action.activate.connect (() => {
			var dialog = new Gth.SortFoldersDialog (this);
			dialog.present (this);
		});
		action_group.add_action (action);

		action = new SimpleAction ("new-window", null);
		action.activate.connect (() => {
			var new_window = new Gth.Window ();
			new_window.browser.open_location (folder_tree.current_folder.file, LoadAction.OPEN, window.get_current_file ());
			new_window.present ();
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("show-hidden-files", null, new Variant.boolean (show_hidden_files));
		action.activate.connect ((_action, param) => {
			set_show_hidden (Util.toggle_state (_action));
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("show-browser-sidebar", null, new Variant.boolean (main_view.show_sidebar));
		action.activate.connect ((_action, param) => {
			main_view.show_sidebar = Util.toggle_state (_action);
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("browser-properties", null, new Variant.boolean (content_view.show_sidebar));
		action.activate.connect ((_action, param) => {
			content_view.show_sidebar = Util.toggle_state (_action);
		});
		action_group.add_action (action);

		action = new SimpleAction ("load-home", null);
		action.activate.connect (() => open_home ());
		action_group.add_action (action);

		action = new SimpleAction ("show-folders", null);
		action.activate.connect ((_action, param) => {
			if (sidebar_state == SidebarState.FILES) {
				return;
			}
			if (last_folder != null) {
				open_location (last_folder);
			}
			else {
				open_home ();
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("show-catalogs", null);
		action.activate.connect ((_action, param) => {
			if (sidebar_state == SidebarState.CATALOGS) {
				return;
			}
			if (last_catalog == null) {
				last_catalog = File.new_for_uri ("catalog:///");
			}
			open_location (last_catalog);
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("load-history-position", VariantType.INT16, new Variant.int16 ((int16) history.current));
		action.activate.connect ((_action, param) => {
			history.load (param.get_int16 ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("delete-history", null);
		action.activate.connect (() => history.clear ());
		action_group.add_action (action);

		action = new SimpleAction ("load-previous", null);
		action.activate.connect (() => history.load_previous ());
		action_group.add_action (action);

		action = new SimpleAction ("load-next", null);
		action.activate.connect (() => history.load_next ());
		action_group.add_action (action);

		action = new SimpleAction ("load-parent", null);
		action.activate.connect ((_action, param) => {
			if (folder_tree.current_folder != null) {
				var parent = folder_tree.current_folder.file.get_parent ();
				if (parent != null) {
					open_location (parent);
				}
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("load-location", VariantType.STRING);
		action.activate.connect ((_action, param) => {
			open_location (File.new_for_uri (param.get_string ()));
		});
		action_group.add_action (action);

		action = new SimpleAction ("search", null);
		action.activate.connect ((_action, param) => {
			new_search.begin ();
			//var dialog = new Gth.SearchDialog (folder_tree.current_folder.file);
			//dialog.transient_for = window;
			//dialog.present ();
			//dialog.focus_first_rule ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("edit-catalog", null);
		action.activate.connect ((_action, param) => {
			edit_catalog.begin ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("update-search", null);
		action.activate.connect ((_action, param) => {
			update_search.begin ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("edit-bookmarks", null);
		action.activate.connect ((_action, param) => {
			var dialog = new Gth.BookmarksDialog ();
			dialog.present (this);
		});
		action_group.add_action (action);

		action = new SimpleAction ("add-bookmark", null);
		action.activate.connect ((_action, param) => {
			app.bookmarks.add_bookmark.begin (folder_tree.current_folder.file, (_obj, res) => {
				try {
					app.bookmarks.add_bookmark.end (res);
				}
				catch (Error error) {
					window.show_error (error);
				}
			});
		});
		action_group.add_action (action);

		action = new SimpleAction ("view-previous", null);
		action.activate.connect (() => view_previous_file ());
		action_group.add_action (action);

		action = new SimpleAction ("view-next", null);
		action.activate.connect (() => view_next_file ());
		action_group.add_action (action);

		action = new SimpleAction ("reload", null);
		action.activate.connect (() => reload ());
		action_group.add_action (action);

		action = new SimpleAction ("delete-files", null);
		action.activate.connect (() => {
			var files = get_selected_file_data_list ();
			if (files.is_empty ()) {
				return;
			}
			var local_job = window.new_job (_("Deleting Files"), JobFlags.FOREGROUND);
			window.file_manager.delete_files.begin (files, local_job, (_obj, res) => {
				try {
					window.file_manager.delete_files.end (res);
				}
				catch (Error error) {
					window.show_error (error);
				}
				finally {
					local_job.done ();
				}
			});
		});
		action_group.add_action (action);

		action = new SimpleAction ("trash-files", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				return;
			}
			var local_job = window.new_job (_("Deleting Files"), JobFlags.FOREGROUND);
			window.file_manager.trash_files.begin (files, local_job, (_obj, res) => {
				try {
					window.file_manager.trash_files.end (res);
				}
				catch (Error error) {
					window.show_error (error);
				}
				finally {
					local_job.done ();
				}
			});
		});
		action_group.add_action (action);

		action = new SimpleAction ("copy-files-to", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				return;
			}
			var local_job = window.new_job (_("Copying Files"), JobFlags.FOREGROUND);
			window.file_manager.copy_files_ask_destination.begin (files, local_job, (_obj, res) => {
				try {
					window.file_manager.copy_files_ask_destination.end (res);
				}
				catch (Error error) {
					window.show_error (error);
				}
				finally {
					local_job.done ();
				}
			});
		});
		action_group.add_action (action);

		action = new SimpleAction ("move-files-to", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			if (files.is_empty ()) {
				return;
			}
			var local_job = window.new_job (_("Moving Files"), JobFlags.FOREGROUND);
			window.file_manager.move_files_ask_destination.begin (files, local_job, (_obj, res) => {
				try {
					window.file_manager.move_files_ask_destination.end (res);
				}
				catch (Error error) {
					window.show_error (error);
				}
				finally {
					local_job.done ();
				}
			});
		});
		action_group.add_action (action);

		action = new SimpleAction ("copy-files", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			copy_files_to_clipboard (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("cut-files", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			cut_files_to_clipboard (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("paste-files", null);
		action.activate.connect (() => {
			var local_job = window.new_job (_("Pasting Files"), JobFlags.FOREGROUND);
			paste_files_from_clipboard.begin (local_job, (_obj, res) => {
				try {
					paste_files_from_clipboard.end (res);
				}
				catch (Error error) {
					window.show_error (error);
				}
				finally {
					local_job.done ();
				}
			});
		});
		action_group.add_action (action);

		action = new SimpleAction ("duplicate-files", null);
		action.activate.connect (() => {
			var source = app.get_source_for_file (folder_tree.current_folder.file);
			if (!(source is FileSourceVfs)) {
				window.show_error (new IOError.FAILED (_("Cannot move files here")));
				return;
			}
			var files = get_selected_files ();
			if (files.is_empty ()) {
				return;
			}
			var local_job = window.new_job (_("Duplicating Files"), JobFlags.FOREGROUND);
			window.file_manager.duplicate_files.begin (files, folder_tree.current_folder.file, local_job, (_obj, res) => {
				try {
					window.file_manager.duplicate_files.end (res);
				}
				catch (Error error) {
					window.show_error (error);
				}
				finally {
					local_job.done ();
				}
			});
		});
		action_group.add_action (action);

		action = new SimpleAction ("select-all", null);
		action.activate.connect (() => {
			grid_model.select_all ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("view-new-window", null);
		action.activate.connect (() => {
			var file = get_selected_file ();
			if (file == null) {
				window.show_error (new IOError.FAILED (_("No file selected")));
				return;
			}
			var new_window = new Gth.Window ();
			new_window.viewer.open_file.begin (file);
			new_window.present ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("view-fullscreen", null);
		action.activate.connect (() => {
			var position = get_selected_position ();
			if (position != uint.MAX) {
				view_position (position, ViewFlags.FULLSCREEN | ViewFlags.NO_DELAY);
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("edit-metadata", null);
		action.activate.connect (() => {
			var file_data = get_selected_file_data ();
			if (file_data == null) {
				window.show_message (_("No file selected"));
				return;
			}
			var dialog = new EditMetadata ();
			var local_job = window.new_job (_("Edit Comment"), JobFlags.FOREGROUND, "gth-note-symbolic");
			dialog.edit.begin (window, file_data, local_job, (_obj, res) => {
				try {
					dialog.edit.end (res);
				}
				catch (Error error) {
					window.show_error (error);
				}
				finally {
					local_job.done ();
				}
			});
		});
		action_group.add_action (action);
	}

	void init_folder_tree () {
		folder_tree.job_queue = window.jobs;
		folder_tree.load.connect ((location, action) => {
			load_folder.begin (location, action);
		});
	}

	Gtk.SortListModel file_sort_model;
	Gtk.FilterListModel file_filter_model;

	void init_file_grid () {
		file_filter = new Gth.FileFilter (this);
		file_filter_model = new Gtk.FilterListModel (folder_tree.current_children.model, file_filter);

		file_sorter = new Gth.FileSorter (this);
		file_sort_model = new Gtk.SortListModel (file_filter_model, file_sorter);

		visible_files = new IterableList<FileData> ((ListModel) file_sort_model);

		file_grid.model = new Gtk.MultiSelection (file_sort_model);
		file_grid.model.selection_changed.connect (() => {
			update_selection_info ();
		});

		var factory = new Gtk.SignalListItemFactory ();
		factory.setup.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			list_item.child = new Gth.FileListItem (this, thumbnail_attributes_v);
		});
		factory.teardown.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			list_item.child = null;
		});
		factory.bind.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			var file_item = list_item.child as Gth.FileListItem;
			var file_data = list_item.item as FileData;
			if (file_data != null) {
				file_item.bind (file_data);
				//thumbnailer.requested_size = (uint) thumbnail_size;
				//thumbnailer.add (file_data);
				//stdout.printf ("  BINDED: %u\n", binded_grid_items.length);
				if (file_item.parent != null) {
					file_item.parent.halign = Gtk.Align.CENTER;
				}
				binded_grid_items.add (list_item);
				thumbnailer.queue_load_next ();
				if (binded_grid_items.length < 50) {
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
					file_data.remove_thumbnail ();
				}
			}
			binded_grid_items.remove (list_item);
		});
		file_grid.factory = factory;

		file_grid.vadjustment.changed.connect (() => { after_grid_vadj_changed (); });
		file_grid.vadjustment.value_changed.connect (() => { after_grid_vadj_changed (); });

		var click_events = new Gtk.GestureClick ();
		click_events.set_button (Gdk.BUTTON_SECONDARY);
		click_events.pressed.connect ((n_press, x, y) => {
			open_context_menu ((int) x, (int) y);
		});
		non_empty_folder.add_controller (click_events);

		click_events = new Gtk.GestureClick ();
		click_events.set_button (Gdk.BUTTON_SECONDARY);
		click_events.pressed.connect ((n_press, x, y) => {
			open_context_menu ((int) x, (int) y);
		});
		empty_folder.add_controller (click_events);
	}

	public Gth.FileData? get_next_file_for_thumbnailer () {
		// stdout.printf ("\n>>>> get_next_file_for_thumbnailer\n\n");
		var top = 0;
		var bottom = file_grid.vadjustment.get_page_size ();
		foreach (unowned Gtk.ListItem item in binded_grid_items) {
			if (!item.child.get_mapped ())
				continue;
			Graphene.Rect bounds;
			if (item.child.compute_bounds (file_grid, out bounds)) {
				var file_data = item.item as FileData;
				if (bounds.origin.x < 0) {
					continue;
				}
				if (thumbnailer.already_added (file_data))
					continue;

				//stdout.printf ("> (%f,%f)[%f,%f] (state: %s) <=> [%f,%f]\n",
				//	bounds.origin.x, bounds.origin.y,
				//	bounds.size.width, bounds.size.height,
				//	file_data.thumbnail_state.to_string (),
				//	top, bottom);

				if ((bounds.origin.y < top - bounds.size.height) || (bounds.origin.y > bottom))
					continue;

				//stdout.printf ("> y: %f, height %f (%s) <=> [%f,%f]\n",
				//	bounds.origin.y,
				//	bounds.size.height,
				//	file_data.file.get_basename (),
				//	top, bottom);

				return file_data;
			}
		}
		return null;
	}

	void after_grid_vadj_changed () {
		thumbnailer.queue_load_next ();
	}

	Job search_job = null;

	async void new_search () {
		if (search_job != null) {
			search_job.cancel ();
		}
		var local_job = window.new_job (_("Searching Files"),
			JobFlags.FOREGROUND,
			"gth-search-symbolic");
		search_job = local_job;
		try {
			local_job.opens_dialog ();
			var editor = new Gth.SearchEditor ();
			var catalog = yield editor.new_search (window,
				folder_tree.current_folder.file,
				local_job.cancellable);
			yield catalog.save_async (local_job.cancellable);
			local_job.dialog_closed ();

			var search = new UpdateSearch ();
			yield search.update_file (this, catalog.file, local_job);
		}
		catch (Error error) {
			local_job.dialog_closed ();
			window.show_error (error);
		}
		finally {
			local_job.done ();
			if (local_job == search_job) {
				search_job = null;
			}
		}
	}

	async void edit_catalog () {
		if (search_job != null) {
			search_job.cancel ();
		}
		var file_data = folder_tree.current_folder;
		var local_job = window.new_job (_("Updating %s").printf (file_data.get_display_name ()),
			JobFlags.FOREGROUND,
			"gth-search-symbolic");
		search_job = local_job;
		try {
			local_job.opens_dialog ();
			var editor = new Gth.CatalogEditor ();
			var catalog = yield editor.edit_catalog (window, file_data.file, local_job.cancellable);
			local_job.dialog_closed ();
			yield catalog.save_async (local_job.cancellable);

			if (editor.search_parameters_changed ()) {
				// Start searching if the search parameters changed.
				var search = new UpdateSearch ();
				yield search.update_file (this, catalog.file, local_job);
			}
		}
		catch (Error error) {
			local_job.dialog_closed ();
			window.show_error (error);
		}
		finally {
			local_job.done ();
			if (local_job == search_job) {
				search_job = null;
			}
		}
	}

	async void update_search () {
		if (search_job != null) {
			search_job.cancel ();
		}
		var file_data = folder_tree.current_folder;
		var local_job = window.new_job (_("Updating %s").printf (file_data.get_display_name ()),
			JobFlags.FOREGROUND,
			"gth-search-symbolic");
		search_job = local_job;
		try {
			var search = new UpdateSearch ();
			yield search.update_file (this, file_data.file, local_job);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
			if (search_job == local_job) {
				search_job = null;
			}
		}
	}

	public void release_resources () {
		// signals.disconnect_all ();
	}

	public void save_preferences (bool page_visible) {
		if (!window.maximized && !window.fullscreened && window.get_mapped ()) {
			int width, height;
			window.get_default_size (out width, out height);
			app.settings.set_int (PREF_BROWSER_WINDOW_WIDTH, width);
			app.settings.set_int (PREF_BROWSER_WINDOW_HEIGHT, height);
		}

		app.settings.set_boolean (PREF_BROWSER_WINDOW_MAXIMIZED, window.maximized);
		app.settings.set_boolean (PREF_BROWSER_SIDEBAR_VISIBLE, main_view.show_sidebar);
		app.settings.set_int (PREF_BROWSER_SIDEBAR_WIDTH, (int) main_view.max_sidebar_width);
		app.settings.set_boolean (PREF_BROWSER_PROPERTIES_VISIBLE, content_view.show_sidebar);
		if (page_visible) {
			app.settings.set_int (PREF_BROWSER_PROPERTIES_WIDTH, int.min ((int) content_view.max_sidebar_width, MAX_SIDEBAR_WIDTH));
		}

		if (app.settings.get_boolean (PREF_BROWSER_GO_TO_LAST_LOCATION)
			&& (folder_tree.current_folder != null))
		{
			var uri = folder_tree.current_folder.file.get_uri ();
			app.settings.set_string (PREF_BROWSER_STARTUP_LOCATION, uri);
		}

		if (page_visible) {
			var selected_file = get_selected_file ();
			if (selected_file != null) {
				app.settings.set_string (PREF_BROWSER_STARTUP_CURRENT_FILE, selected_file.get_uri ());
			}
			else {
				app.settings.set_string (PREF_BROWSER_STARTUP_CURRENT_FILE, "");
			}
		}

		if (file_sorter.name != null) {
			app.settings.set_string (PREF_BROWSER_SORT_TYPE, app.migration.test.get_old_key (file_sorter.name));
			app.settings.set_boolean (PREF_BROWSER_SORT_INVERSE, file_sorter.inverse);
		}

		if (folder_tree.sort.name != null) {
			app.settings.set_string (PREF_BROWSER_FOLDER_TREE_SORT_TYPE, folder_tree.sort.name);
			app.settings.set_boolean (PREF_BROWSER_FOLDER_TREE_SORT_INVERSE, folder_tree.sort.inverse);
		}

		filter_bar.save_to_file ();
		history.save_to_file ();
	}

	[GtkCallback]
	void on_file_activate (uint position) {
		var flags = ViewFlags.NO_DELAY;
		if (open_in_fullscreen) {
			flags |= ViewFlags.FULLSCREEN;
		}
		view_position (position, flags);
	}

	public bool view_position (uint position, ViewFlags flags = ViewFlags.DEFAULT) {
		var file = file_grid.model.get_item (position) as FileData;
		if (file == null) {
			return false;
		}
		window.viewer.set_file_position (position);
		window.viewer.view_file (file, flags);
		return true;
	}

	public void view_previous_file () {
		if (window.viewer.position > 0) {
			view_position (window.viewer.position - 1);
		}
		else {
			window.edge_reached ();
		}
	}

	public void view_next_file () {
		if (!view_position (window.viewer.position + 1)) {
			window.edge_reached ();
		}
	}

	public void set_thumbnail_size (int size) {
		if (size == thumbnail_size)
			return;
		thumbnail_size = size;
		thumbnailer.requested_size = (uint) thumbnail_size;
		foreach (unowned Gtk.ListItem item in binded_grid_items) {
			unowned var file_item = item.child as Gth.FileListItem;
			file_item.set_size (thumbnail_size);
		}
		thumbnailer.queue_load_next ();
	}

	public void add_to_search_results (File parent, FileData file_data) {
		if (!folder_tree.current_folder.file.equal (parent)) {
			return;
		}

		folder_tree.current_children.model.append (file_data);
		file_filter.reset ();
		if ((file_filter.filter != null) && !file_filter.filter.match (file_data)) {
			return;
		}

		total_files++;
		total_size += file_data.info.get_size ();

		status.set_list_info (total_files, total_size);
		window.viewer.set_list_info (total_files);
		if (total_files == 1) {
			folder_stack.set_visible_child (non_empty_folder);
		}
	}

	public void catalog_saved (Gth.Catalog catalog, File old_file) {
		if (folder_tree.current_folder.file.equal (old_file)) {
			catalog.update_file_info (folder_tree.current_folder.info);
			folder_tree.current_folder.set_file (catalog.file);
			folder_tree.current_folder.info_changed ();
			update_title ();
		}
		var iter = new TreeIterator<Gth.FileData> (folder_tree.tree_model);
		while (iter.next ()) {
			var child = iter.get_data ();
			if (child.file.equal (old_file)) {
				child.set_file (catalog.file);
				catalog.update_file_info (child.info);
				child.info_changed ();
				child.renamed ();
				break;
			}
		}
	}

	int browser_width = -1;
	int browser_height = -1;

	public void save_window_size () {
		browser_width = window.get_width ();
		browser_height = window.get_height ();
	}

	public void restore_window_size () {
		if ((browser_width != -1) && (browser_height != -1)) {
			window.set_default_size (browser_width, browser_height);
		}
	}

	public void files_created (File parent, GenericList<File> files) {
		if (!folder_tree.current_folder.file.equal (parent)) {
			return;
		}
		var new_files = new GenericList<File>();
		foreach (unowned var file in files) {
			var iter = folder_tree.current_children.iterator ();
			var pos = iter.find_first ((file_data) => file_data.file.equal (file));
			if (pos < 0) {
				new_files.model.append (file);
			}
		}
		if (!new_files.is_empty ()) {
			add_files.begin (new_files);
		}
	}

	public void files_deleted (GenericList<File> files) {
		foreach (unowned var file in files) {
			var iter = folder_tree.current_children.iterator ();
			var pos = iter.find_first ((file_data) => file_data.file.equal (file));
			if (pos >= 0) {
				folder_tree.current_children.model.remove ((uint) pos);
			}
			if ((property_sidebar.current_file != null)
				&& property_sidebar.current_file.file.equal (file))
			{
				property_sidebar.current_file = null;
			}
		}
	}

	void copy_files_to_clipboard (GenericList<File> files) {
		var text = new StringBuilder ();
		var uri_list = new StringBuilder ();
		foreach (unowned var file in files) {
			if (text.len > 0) {
				text.append ("\n");
			}
			if (file.get_uri_scheme () == "file") {
				text.append (file.get_path ());
			}
			else {
				text.append (file.get_uri ());
			}

			if (uri_list.len > 0) {
				uri_list.append ("\n");
			}
			uri_list.append (file.get_uri ());
		}

		unowned var clipboard = get_clipboard ();
		var text_provider = new Gdk.ContentProvider.for_bytes ("text/plain", new Bytes (text.str.data));
		var uri_provider = new Gdk.ContentProvider.for_bytes ("text/uri-list", new Bytes (uri_list.str.data));
		var provider = new Gdk.ContentProvider.union ({ text_provider, uri_provider });
		clipboard.set_content (provider);
	}

	void cut_files_to_clipboard (GenericList<File> files) {
		var uri_list = new StringBuilder ();
		foreach (unowned var file in files) {
			if (uri_list.len > 0) {
				uri_list.append ("\n");
			}
			uri_list.append (file.get_uri ());
		}

		unowned var clipboard = get_clipboard ();
		var provider = new Gdk.ContentProvider.for_bytes ("gthumb/cut-files", new Bytes (uri_list.str.data));
		clipboard.set_content (provider);
	}

	async void paste_files_from_clipboard (Job job) throws Error {
		unowned var clipboard = get_clipboard ();
		unowned string mime_type;
		var stream = yield clipboard.read_async (
			{ "text/uri-list", "gthumb/cut-files" },
			Priority.DEFAULT,
			job.cancellable,
			out mime_type);
		var bytes = yield Files.read_all_async (stream, job.cancellable);
		unowned var data_as_string = (string) bytes.get_data ();
		var text = data_as_string.ndup (bytes.length);
		//stdout.printf ("> text: '%s'\n", text);
		//stdout.printf ("> type: '%s'\n", mime_type);
		var uris = text.split_set ("\n\r");
		var files = new GenericList<File> ();
		foreach (unowned var uri in uris) {
			//stdout.printf ("> URI: %s\n", uri);
			files.model.append (File.new_for_uri (uri));
		}
		if (mime_type == "gthumb/cut-files") {
			var source = app.get_source_for_file (folder_tree.current_folder.file);
			if (!(source is FileSourceVfs)) {
				throw new IOError.FAILED (_("Cannot move files here"));
			}
			yield window.file_manager.move_files (files,
				folder_tree.current_folder.file,
				job);
		}
		else {
			var source = app.get_source_for_file (folder_tree.current_folder.file);
			yield source.copy_files (window,
				files,
				folder_tree.current_folder.file,
				job);
		}
	}

	[GtkChild] unowned Adw.OverlaySplitView main_view;
	[GtkChild] public unowned Adw.OverlaySplitView content_view;
	[GtkChild] public unowned Gth.FilterBar filter_bar;
	[GtkChild] unowned Gtk.MenuButton app_menu_button;
	[GtkChild] unowned Gth.ActionPopover bookmark_popover;
	[GtkChild] unowned Gth.ActionPopover history_popover;
	[GtkChild] unowned Gtk.GridView file_grid;
	[GtkChild] unowned Gtk.Stack folder_stack;
	[GtkChild] unowned Gtk.Widget non_empty_folder;
	[GtkChild] public unowned Adw.StatusPage empty_folder;
	[GtkChild] unowned Gtk.ToggleButton vfs_button;
	[GtkChild] unowned Gtk.ToggleButton catalog_button;
	[GtkChild] public unowned Adw.ToastOverlay toast_overlay;
	[GtkChild] unowned Gtk.Stack sidebar_stack;
	//[GtkChild] unowned Gtk.Stack second_sidebar_stack;
	[GtkChild] public unowned Gth.PropertySidebar property_sidebar;
	[GtkChild] unowned Gtk.MenuButton location_button;
	[GtkChild] unowned Gth.ActionList location_actions;
	[GtkChild] unowned Gth.SidebarResizer sidebar_resizer;
	[GtkChild] unowned Gtk.Button edit_catalog_button;
	[GtkChild] unowned Gtk.Button update_search_button;
	[GtkChild] unowned Gtk.Button update_folder_button;
	[GtkChild] unowned Gtk.PopoverMenu file_context_menu;
	[GtkChild] unowned Gtk.PopoverMenu context_menu;
	[GtkChild] public unowned Gth.FolderStatus folder_status;
	[GtkChild] public unowned Gth.ActionPopover tools_popover;

	Gth.Test general_filter;
	Gth.Thumbnailer thumbnailer;
	Gth.History history;
	weak Window _window;
	Queue<File> current_parents;
	File last_folder = null;
	File last_catalog = null;
	GenericArray<Gtk.ListItem> binded_grid_items;
	ActionCategory actions_category;
	ActionCategory bookmarks_category;
	ActionCategory parents_category;
	uint total_files = 0;
	uint64 total_size = 0;
}

public class Gth.FileSorter : Gtk.Sorter {
	public string name;
	public bool inverse;
	public weak Gth.SortInfo? sort_info;

	public FileSorter (Browser _browser) {
		name = null;
		inverse = false;
		sort_info = null;
		browser = _browser;
	}

	public void set_order (string _name, bool _inverse) {
		name = _name;
		inverse = _inverse;
		sort_info = app.get_sorter_by_id (name);
		if (sort_info == null) {
			sort_info = app.get_sorter_by_id ("File::Name");
		}
		changed (Gtk.SorterChange.DIFFERENT);
	}

	public void set_inverse (bool _inverse) {
		if (inverse == _inverse) {
			return;
		}
		inverse = _inverse;
		changed (Gtk.SorterChange.INVERTED);
	}

	public int cmp_func (Object a, Object b) {
		var result = sort_info.cmp_func ((FileData) a, (FileData) b);
		return inverse ? -result : result;
	}

	public override Gtk.Ordering compare (Object? a, Object? b) {
		var result = sort_info.cmp_func ((FileData) a, (FileData) b);
		if (result == 0) {
			return Gtk.Ordering.EQUAL;
		}
		if (inverse) {
			result = -result;
		}
		return (result < 0) ? Gtk.Ordering.SMALLER : Gtk.Ordering.LARGER;
	}

	public override Gtk.SorterOrder get_order () {
		return Gtk.SorterOrder.TOTAL;
	}

	weak Browser browser;
}

public class Gth.FileFilter : Gtk.Filter {
	public Gth.Filter filter;

	public FileFilter (Browser _browser) {
		filter = null;
		browser = _browser;
		visible = new GenericSet<File> (Util.file_hash, Util.file_equal);
	}

	public void update_filter () {
		filter = app.add_general_filter (browser.filter_bar.filter);
		if (!browser.show_hidden_files) {
			filter.tests.add (new Gth.TestVisible ());
		}
		reset ();
	}

	public void reset () {
		visible.remove_all ();
		var iter = filter.iterator (browser.folder_tree.current_children);
		while (iter.next ()) {
			unowned var file_data = iter.get ();
			visible.add (file_data.file);
		}
		changed (Gtk.FilterChange.DIFFERENT);
	}

	public override bool match (Object? item) {
		unowned var file_data = (FileData) item;
		return visible.contains (file_data.file);
	}

	public override Gtk.FilterMatch get_strictness () {
		return Gtk.FilterMatch.SOME;
	}

	weak Browser browser;
	GenericSet<File> visible;
}
