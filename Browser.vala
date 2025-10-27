[GtkTemplate (ui = "/app/gthumb/gthumb/ui/browser.ui")]
public class Gth.Browser : Gtk.Box {
	public weak Window window {
		get { return _window; }
		set {
			_window = value;
			init ();
		}
	}

	public MenuModel folder_menu { set; get; }
	public MenuModel catalog_menu { set; get; }
	public MenuModel selection_menu { set; get; }

	enum SidebarState {
		NONE,
		FILES,
		CATALOGS,
		SELECTIONS,
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
	public SimpleActionGroup folder_actions;
	public SimpleActionGroup catalog_actions;
	public bool reordering = false;

	construct {
		history = new History (this);
		current_parents = null;
		binded_grid_items = new GenericArray<Gtk.ListItem> ();
		actions_category = new ActionCategory ("", -1);
		bookmarks_category = new ActionCategory (_("Bookmarks"), 1);
		parents_category = new ActionCategory (_("Path"), 1);
		never_loaded = true;
		folder_stack.set_visible_child (empty_folder);
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
			window.action_group.change_action_state ("browser-sidebar", new Variant.boolean (main_view.show_sidebar));
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

		view_properties.visible = !(content_view.show_sidebar && !content_view.collapsed);

		init_actions ();
		init_folder_actions ();
		init_catalog_actions ();

		content_view.notify["show-sidebar"].connect (() => {
			if (!content_view.show_sidebar) {
				Util.set_active (window.action_group, "browser-properties-pinned", false);
				Util.set_active (window.action_group, "browser-properties-collapsed", false);
			}
		});
	}

	async void add_files (GenericList<File> files) {
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
		file_filter.after_adding_files ();
		update_total_files ();
	}

	public async void load_folder (File location, LoadAction load_action, Job? job = null) throws Error {
		thumbnailer.cancel ();
		try {
			freeze_thumbnail_list ();
			folder_tree.list_attributes = get_list_attributes (true);
			yield folder_tree.load_folder (location, load_action, job);
		}
		finally {
			thaw_thumbnail_list ();
		}
		update_title ();
		update_load_sensitivity ();
		update_location_commands ();
		var is_catalog = folder_tree.current_folder.file.has_uri_scheme ("catalog");
		var is_selection = folder_tree.current_folder.file.has_uri_scheme ("selection");
		set_sidebar_state (is_catalog ? SidebarState.CATALOGS : is_selection ? SidebarState.SELECTIONS : SidebarState.FILES);
		if (is_catalog) {
			last_catalog = folder_tree.current_folder.file;
			folder_tree.menu_model = catalog_menu;
		}
		else if (is_selection) {
			last_selection = folder_tree.current_folder.file;
			folder_tree.menu_model = selection_menu;
		}
		else {
			last_folder = folder_tree.current_folder.file;
			folder_tree.menu_model = folder_menu;
		}
		update_selection_info ();
		update_location_menu ();
		update_free_space_info ();
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
			selection_button.active = false;
			break;
		case SidebarState.CATALOGS:
			sidebar_stack.set_visible_child (folder_tree);
			vfs_button.active = false;
			catalog_button.active = true;
			selection_button.active = false;
			break;
		case SidebarState.SELECTIONS:
			sidebar_stack.set_visible_child (folder_tree);
			vfs_button.active = false;
			catalog_button.active = false;
			selection_button.active = true;
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
		var file_data = folder_tree.current_folder;
		var is_catalog = false;
		var is_search = false;
		var uri = file_data.file.get_uri ();
		var source_type = app.get_source_type_for_uri (uri);
		if (file_data.file.has_uri_scheme ("catalog")) {
			is_catalog = uri.has_suffix (".catalog");
			is_search = uri.has_suffix (".search");
		}
		var is_selection = file_data.file.has_uri_scheme ("selection");
		var can_write = file_data.info.get_attribute_boolean (FileAttribute.ACCESS_CAN_WRITE);
		var is_vfs_folder = (source_type == typeof (FileSourceVfs));

		edit_catalog_button.visible = is_catalog || is_search;
		update_search_button.visible = is_search;
		update_folder_button.visible = is_vfs_folder;
		Util.enable_action (window.action_group, "remove-from-catalog", is_catalog || is_search);

		var is_reorderable = folder_tree.current_source.is_reorderable ();
		reorder_button.visible = is_reorderable;

		Util.enable_action (window.action_group, "cut-files", can_write);
		Util.enable_action (window.action_group, "paste-files", can_write && window.can_paste_from_clipboad);
		Util.enable_action (window.action_group, "move-files-to", can_write);
		Util.enable_action (window.action_group, "rename-files", can_write);
		Util.enable_action (window.action_group, "duplicate-files", can_write);
		Util.enable_action (window.action_group, "trash-files", can_write);
		Util.enable_action (window.action_group, "delete-files-from-disk", can_write);
		Util.enable_action (window.action_group, "remove-files", can_write);
		Util.enable_action (window.action_group, "remove-from-current-selection", is_selection);
		Util.enable_action (window.action_group, "open-terminal-here", is_vfs_folder);
		Util.enable_action (window.action_group, "open-file-manager-here", is_vfs_folder);
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
		if (file_filter.filter.has_attributes ()) {
			attributes.append (",");
			attributes.append (file_filter.filter.attributes);
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
		update_selection_info ();
		if (list_attributes != null) {
			var list_attribute_valid = Util.attributes_match_any_pattern (filter_bar.filter.attributes, list_attributes);
			if (!list_attribute_valid) {
				// The filter requires metadata that was not loaded, reload
				// the current location to get the filter metadata as well.
				reload ();
			}
		}
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
		location_label.label = title;
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
		file_sorter.reset ();
		file_filter_model.model = filtered_model;
		file_sort_model.model = sorted_model;
		file_grid.model = grid_model;
		update_total_files ();
		focus_thumbnail_list ();
	}

	public void update_folder_status () {
		folder_stack.set_visible_child ((total_files == 0) ? empty_folder : non_empty_folder);
		if (total_files == 0) {
			if ((filter_bar.filter != null) && (filter_bar.filter.id != "")) {
				folder_status.title = _("No Matches");
				folder_status.description = "";
				folder_status.icon_name = "gth-filter-symbolic";
			}
			else if (folder_tree.current_folder != null) {
				folder_status.gicon = folder_tree.current_folder.get_symbolic_icon ();
				switch (folder_tree.current_folder.get_content_type ()) {
				case "gthumb/search":
					folder_status.title = _("No Files Found");
					folder_status.description = "";
					break;
				case "gthumb/catalog":
					folder_status.title = _("Empty Catalog");
					folder_status.description = "";
					break;
				case "gthumb/selection":
					folder_status.title = _("Empty Selection");
					var str = new StringBuilder ();
					var number = Selection.get_number (folder_tree.current_folder.file);
					var action = "win.add-to-selection('%u')".printf (number);
					var shortcut = app.shortcuts.by_action.get (action);
					if (shortcut != null) {
						// Translators: %s is replaced by a keyword shortcut, %u is a number (1 2 or 3).
						str.append_printf (_("Use %s to add files to selection %u"), shortcut.accelerator_label, number);
					}
					action = "win.open-selection(%u)".printf (number);
					shortcut = app.shortcuts.by_action.get (action);
					if (shortcut != null) {
						if (str.len > 0) {
							str.append ("\n\n");
						}
						// Translators: %s is replaced by a keyword shortcut, %u is a number (1 2 or 3).
						str.append_printf (_("Use %s to view selection %u"), shortcut.accelerator_label, number);
					}
					folder_status.description = str.str;
					break;
				case "gthumb/selections":
					folder_status.title = folder_tree.current_folder.get_display_name ();
					folder_status.description = "";
					break;
				case "gthumb/library":
					folder_status.title = folder_tree.current_folder.get_display_name ();
					folder_status.description = "";
					break;
				default:
					folder_status.title = _("Empty Folder");
					folder_status.description = "";
					break;
				}
			}
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

	public int get_file_position (File file) {
		var iter = visible_files.iterator ();
		return iter.find_first ((file_data) => file_data.file.equal (file));
	}

	public void select_position (int position, SelectFile flags = SelectFile.DEFAULT) {
		if (position < 0) {
			return;
		}
		if (SelectFile.SCROLL_TO_FILE in flags) {
			file_grid.scroll_to ((uint) position, Gtk.ListScrollFlags.SELECT | Gtk.ListScrollFlags.FOCUS, null);
		}
		else {
			file_grid.model.select_item ((uint) position, true);
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
					file_grid.model.select_item ((uint) position, false);
				}
			}
		}
	}

	Gth.Job free_space_job = null;

	void update_free_space_info () {
		if (free_space_job != null) {
			free_space_job.cancel ();
			free_space_job = null;
		}
		if (!(folder_tree.current_source is FileSourceVfs)) {
			status.set_free_space_info (0);
			return;
		}
		var local_job = window.new_job ("Free Space");
		free_space_job = local_job;
		var location = folder_tree.current_folder.file;
		location.query_filesystem_info_async.begin (FileAttribute.FILESYSTEM_FREE, Priority.DEFAULT, local_job.cancellable, (_obj, res) => {
			try {
				var info = location.query_filesystem_info_async.end (res);
				var free_space = info.get_attribute_uint64 (FileAttribute.FILESYSTEM_FREE);
				status.set_free_space_info (free_space);
			}
			catch (Error error) {
				// Ignore
			}
			finally {
				local_job.done ();
				if (local_job == free_space_job) {
					free_space_job = null;
				}
			}
		});
	}

	Gth.Job sidebar_job = null;

	void update_selection_info () {
		if (sidebar_job != null) {
			sidebar_job.cancel ();
			sidebar_job = null;
		}
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
			sidebar_job = local_job;
			property_sidebar.load.begin (selected_file, local_job.cancellable, (_obj, res) => {
				try {
					property_sidebar.load.end (res);
				}
				catch (Error error) {
					stdout.printf ("ERROR: %s\n", error.message);
				}
				finally {
					local_job.done ();
					if (local_job == sidebar_job) {
						sidebar_job = null;
					}
				}
			});
		}
		else {
			//property_sidebar.current_file = null;
			property_sidebar.set_selection_info (total_files, total_size);
		}
		status.set_selection_info (total_files, total_size);

		var can_open_container = (total_files == 1) && !(folder_tree.current_source is FileSourceVfs);
		Util.enable_action (window.action_group, "open-container", can_open_container);
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

		action = new SimpleAction ("reset-filter", null);
		action.activate.connect (() => filter_bar.reset_filter ());
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

		action = new SimpleAction.stateful ("hidden-files", null, new Variant.boolean (show_hidden_files));
		action.activate.connect ((_action, param) => {
			set_show_hidden (Util.toggle_state (_action));
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("browser-sidebar", null, new Variant.boolean (main_view.show_sidebar));
		action.activate.connect ((_action, param) => {
			main_view.show_sidebar = Util.toggle_state (_action);
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("browser-properties-pinned", null, new Variant.boolean (content_view.show_sidebar && !content_view.collapsed));
		action.activate.connect ((_action, param) => {
			var show = Util.toggle_state (_action);
			if (show) {
				content_view.collapsed = false;
			}
			Util.set_active (window.action_group, "browser-properties-collapsed", false);
			view_properties.visible = !show;
			content_view.show_sidebar = show;
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("browser-properties-collapsed", null, new Variant.boolean (content_view.show_sidebar && content_view.collapsed));
		action.activate.connect ((_action, param) => {
			var show = Util.toggle_state (_action);
			if (show) {
				content_view.collapsed = true;
			}
			Util.set_active (window.action_group, "browser-properties-pinned", false);
			content_view.show_sidebar = show;
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

		action = new SimpleAction ("show-selections", null);
		action.activate.connect ((_action, param) => {
			if (sidebar_state == SidebarState.SELECTIONS) {
				return;
			}
			if (last_selection == null) {
				last_selection = File.new_for_uri ("selection:///");
			}
			open_location (last_selection);
		});
		action_group.add_action (action);

		action = new SimpleAction ("show-bookmarks", null);
		action.activate.connect ((_action, param) => {
			bookmark_popover.popup ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("show-history", null);
		action.activate.connect ((_action, param) => {
			history_popover.popup ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("show-filters", null);
		action.activate.connect ((_action, param) => {
			filter_bar.show_menu ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("show-path", null);
		action.activate.connect ((_action, param) => {
			location_button.popup ();
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

		action = new SimpleAction ("load-next-folder", null);
		action.activate.connect ((_action, param) => {
			if (folder_tree.current_folder != null) {
				int pos;
				if (folder_tree.get_file_row (folder_tree.current_folder.file, out pos) != null) {
					if (!folder_tree.select_position (pos + 1)) {
						window.edge_reached ();
					}
				}
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("load-prev-folder", null);
		action.activate.connect ((_action, param) => {
			if (folder_tree.current_folder != null) {
				int pos;
				if (folder_tree.get_file_row (folder_tree.current_folder.file, out pos) != null) {
					if (!folder_tree.select_position (pos - 1)) {
						window.edge_reached ();
					}
				}
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("ask-location", null);
		action.activate.connect ((_action, param) => {
			ask_location.begin ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("find", null);
		action.activate.connect ((_action, param) => {
			new_search.begin ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("edit-catalog", null);
		action.activate.connect ((_action, param) => {
			edit_catalog.begin (folder_tree.current_folder);
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

		action = new SimpleAction ("view-first", null);
		action.activate.connect (() => view_first_file ());
		action_group.add_action (action);

		action = new SimpleAction ("view-last", null);
		action.activate.connect (() => view_last_file ());
		action_group.add_action (action);

		action = new SimpleAction ("reload", null);
		action.activate.connect (() => reload ());
		action_group.add_action (action);

		action = new SimpleAction ("copy-files", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			window.copy_files_to_clipboard (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("cut-files", null);
		action.activate.connect (() => {
			var files = get_selected_files ();
			window.cut_files_to_clipboard (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("paste-files", null);
		action.activate.connect (() => {
			var local_job = window.new_job (_("Pasting Files"), JobFlags.FOREGROUND);
			paste_files_from_clipboard.begin (folder_tree.current_folder.file, local_job, (_obj, res) => {
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
			if (!(folder_tree.current_source is FileSourceVfs)) {
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

		action = new SimpleAction ("file-list-select-all", null);
		action.activate.connect (() => {
			grid_model.select_all ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("view-fullscreen", null);
		action.activate.connect (() => view_fullscreen ());
		action_group.add_action (action);

		action = new SimpleAction ("focus-thumbnail-list", null);
		action.activate.connect (() => focus_thumbnail_list ());
		action_group.add_action (action);

		action = new SimpleAction ("edit-file", null);
		action.activate.connect (() => edit_file ());
		action_group.add_action (action);

		action = new SimpleAction ("add-to-catalog", null);
		action.activate.connect (() => add_to_catalog.begin ());
		action_group.add_action (action);

		action = new SimpleAction ("remove-from-catalog", null);
		action.activate.connect (() => remove_from_catalog ());
		action_group.add_action (action);

		action = new SimpleAction.stateful ("reorder-files", null, new Variant.boolean (reordering));
		action.activate.connect ((_action, _params) => {
			activate_reordering (Util.toggle_state (_action));
		});
		action_group.add_action (action);

		action = new SimpleAction ("add-to-selection", VariantType.STRING);
		action.activate.connect ((_action, _params) => {
			add_to_selection (uint.parse (_params.get_string (), 10));
		});
		action_group.add_action (action);

		action = new SimpleAction ("remove-from-selection", VariantType.INT32);
		action.activate.connect ((_action, _params) => {
			remove_from_selection ((uint) _params.get_int32 ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("remove-from-current-selection", null);
		action.activate.connect ((_action, _params) => {
			var is_selection = folder_tree.current_folder.file.has_uri_scheme ("selection");
			if (is_selection) {
				var files = get_selected_files ();
				if ((files != null) && !files.is_empty ()) {
					window.remove_files (files);
				}
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-selection", VariantType.INT32);
		action.activate.connect ((_action, _params) => {
			open_selection ((uint) _params.get_int32 ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-container", null);
		action.activate.connect ((_action, _params) => {
			var selected_file = get_selected_file ();
			if (selected_file != null) {
				var parent = selected_file.get_parent ();
				if (parent != null) {
					open_location (parent, LoadAction.OPEN, selected_file);
				}
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-terminal-here", null);
		action.activate.connect ((_action, param) => {
			if (folder_tree.current_folder != null) {
				open_terminal (folder_tree.current_folder.file);
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-file-manager-here", null);
		action.activate.connect ((_action, param) => {
			if (folder_tree.current_folder != null) {
				open_file_manager (folder_tree.current_folder.file);
			}
		});
		action_group.add_action (action);
	}

	public void view_fullscreen () {
		var position = get_selected_position ();
		if (position != uint.MAX) {
			view_position (position, ViewFlags.FULLSCREEN | ViewFlags.NO_DELAY);
		}
	}

	void init_folder_actions () {
		folder_actions = new SimpleActionGroup ();
		var action_group = folder_actions;
		window.insert_action_group ("folder", action_group);

		var action = new SimpleAction ("open-new-window", null);
		action.activate.connect ((_action, param) => {
			if (folder_tree.context_file != null) {
				var new_window = new Gth.Window ();
				new_window.browser.open_location (folder_tree.context_file.file);
				new_window.present ();
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-terminal", null);
		action.activate.connect ((_action, param) => {
			if (folder_tree.context_file != null) {
				open_terminal (folder_tree.context_file.file);
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-file-manager", null);
		action.activate.connect ((_action, param) => {
			if (folder_tree.context_file != null) {
				open_file_manager (folder_tree.context_file.file);
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("new", null);
		action.activate.connect ((_action, param) => {
			new_folder.begin ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("paste", null);
		action.activate.connect ((_action, param) => {
			var local_job = window.new_job (_("Pasting Files"), JobFlags.FOREGROUND);
			paste_files_from_clipboard.begin (folder_tree.context_file.file, local_job, (_obj, res) => {
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

		action = new SimpleAction ("copy", null);
		action.activate.connect (() => {
			var files = new GenericList<File> ();
			files.model.append (folder_tree.context_file.file);
			window.copy_files_to_clipboard (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("cut", null);
		action.activate.connect (() => {
			var files = new GenericList<File> ();
			files.model.append (folder_tree.context_file.file);
			window.cut_files_to_clipboard (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("remove", null);
		action.activate.connect (() => {
			var files = new GenericList<File> ();
			files.model.append (folder_tree.context_file);
			window.remove_files (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("delete-from-disk", null);
		action.activate.connect (() => {
			var files = new GenericList<FileData> ();
			files.model.append (folder_tree.context_file);
			window.delete_files_from_disk (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("copy-to", null);
		action.activate.connect (() => {
			var files = new GenericList<File> ();
			files.model.append (folder_tree.context_file.file);
			window.copy_files_ask_destination (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("move-to", null);
		action.activate.connect (() => {
			var files = new GenericList<File> ();
			files.model.append (folder_tree.context_file.file);
			window.move_files_ask_destination (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("trash", null);
		action.activate.connect (() => {
			var files = new GenericList<File> ();
			files.model.append (folder_tree.context_file.file);
			window.trash_files (files);
		});
		action_group.add_action (action);

		action = new SimpleAction ("rename", null);
		action.activate.connect (() => {
			rename_folder.begin ();
		});
		action_group.add_action (action);
	}

	void init_catalog_actions () {
		catalog_actions = new SimpleActionGroup ();
		var action_group = catalog_actions;
		window.insert_action_group ("catalog", action_group);

		var action = new SimpleAction ("new-catalog", null);
		action.activate.connect ((_action, param) => {
			new_catalog.begin ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("new-library", null);
		action.activate.connect ((_action, param) => {
			new_library.begin ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("rename", null);
		action.activate.connect ((_action, param) => {
			rename_catalog.begin ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("delete", null);
		action.activate.connect ((_action, param) => {
			delete_catalog.begin ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("move-to", null);
		action.activate.connect ((_action, param) => {
			move_catalog.begin ();
		});
		action_group.add_action (action);
	}

	void init_folder_tree () {
		folder_tree.job_queue = window.jobs;
		folder_tree.load.connect ((location, action) => {
			load_folder.begin (location, action);
		});
		folder_tree.before_context_menu_popup.connect ((file_data) => {
			update_folder_context_menu_sensitivity (file_data);
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

		drop_target = new Gtk.DropTarget (typeof (FileListItem), Gdk.DragAction.MOVE);
		drop_target.drop.connect ((value, x, y) => {
			if (!reordering) {
				return false;
			}
			var item = value;
			var source = value.get_object () as FileListItem;
			if (source.browser != this) {
				return false;
			}
			DropSide side;
			var destination = get_item_at (x, y, out side);
			if (destination == null) {
				return false;
			}
			reorder_selected_file ((side == DropSide.LEFT) ? destination.position : destination.position + 1);
			return true;
		});
		drop_target.motion.connect ((x, y) => {
			DropSide side;
			var item = get_item_at (x, y, out side);
			if (item != null) {
				if ((last_drop_target != null) && (last_drop_target.parent != null)) {
					last_drop_target.parent.remove_css_class ("drop-target-left");
					last_drop_target.parent.remove_css_class ("drop-target-right");
				}
				last_drop_target = item.child;
				last_drop_target.parent.add_css_class ("drop-target-" + ((side == DropSide.LEFT) ? "left" : "right"));
			}
			return Gdk.DragAction.MOVE;
		});
		drop_target.leave.connect (() => {
			if (last_drop_target != null) {
				if (last_drop_target.parent != null) {
					last_drop_target.parent.remove_css_class ("drop-target-left");
					last_drop_target.parent.remove_css_class ("drop-target-right");
				}
				last_drop_target = null;
			}
		});
	}

	Gtk.Widget last_drop_target = null;

	public Gth.FileData? get_next_file_for_thumbnailer () {
		// stdout.printf ("\n>>>> get_next_file_for_thumbnailer\n\n");
		// First visible item.
		var top = 0;
		var bottom = file_grid.vadjustment.get_page_size ();
		foreach (unowned Gtk.ListItem item in binded_grid_items) {
			if (!item.child.get_mapped ()) {
				continue;
			}
			Graphene.Rect bounds;
			if (item.child.compute_bounds (file_grid, out bounds)) {
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
			var editor = new Gth.SearchEditor ();
			var catalog = yield editor.new_search (window,
				folder_tree.current_folder.file,
				local_job);
			yield catalog.save_async (local_job.cancellable);
			var search = new UpdateSearch ();
			yield search.update_file (this, catalog.file, local_job);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
			if (local_job == search_job) {
				search_job = null;
			}
		}
	}

	async void edit_catalog (FileData file_data) {
		if (search_job != null) {
			search_job.cancel ();
		}
		var local_job = window.new_job (_("Updating %s").printf (file_data.get_display_name ()),
			JobFlags.FOREGROUND,
			"gth-search-symbolic");
		search_job = local_job;
		try {
			var editor = new Gth.CatalogEditor ();
			var catalog = yield editor.edit_catalog (window, file_data.file, local_job);
			yield catalog.save_async (local_job.cancellable);
			if (editor.search_parameters_changed ()) {
				// Start searching if the search parameters changed.
				var search = new UpdateSearch ();
				yield search.update_file (this, catalog.file, local_job);
			}
		}
		catch (Error error) {
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

	void update_folder_context_menu_sensitivity (FileData file_data) {
		var can_rename = file_data.info.get_attribute_boolean (FileAttribute.ACCESS_CAN_RENAME);
		var can_write = file_data.info.get_attribute_boolean (FileAttribute.ACCESS_CAN_WRITE);
		var can_trash = file_data.info.get_attribute_boolean (FileAttribute.ACCESS_CAN_TRASH);
		var can_delete = file_data.info.get_attribute_boolean (FileAttribute.ACCESS_CAN_DELETE);
		if ((file_data.get_content_type () == "gthumb/catalog")
			|| (file_data.get_content_type () == "gthumb/search")
			|| (file_data.get_content_type () == "gthumb/library"))
		{
			Util.enable_action (catalog_actions, "rename", can_rename);
			Util.enable_action (catalog_actions, "move-to", can_delete);
			Util.enable_action (catalog_actions, "delete", can_delete);
		}
		else {
			Util.enable_action (folder_actions, "new", can_write);
			Util.enable_action (folder_actions, "cut", can_delete);
			Util.enable_action (folder_actions, "paste", can_write && window.can_paste_from_clipboad);
			Util.enable_action (folder_actions, "move-to", can_delete);
			Util.enable_action (folder_actions, "rename", can_rename);
			Util.enable_action (folder_actions, "trash", can_trash);
			Util.enable_action (folder_actions, "delete", can_delete);
		}
	}

	public void release_resources () {
		folder_tree.release_resources ();
	}

	public void save_preferences (bool page_visible) {
		if (!window.maximized && !window.fullscreened && window.get_mapped ()) {
			int width, height;
			window.get_default_size (out width, out height);
			app.settings.set_int (PREF_BROWSER_WINDOW_WIDTH, width);
			app.settings.set_int (PREF_BROWSER_WINDOW_HEIGHT, height);
		}

		app.settings.set_boolean (PREF_BROWSER_WINDOW_MAXIMIZED, window.maximized);
		app.settings.set_boolean (PREF_BROWSER_SIDEBAR_VISIBLE, main_view.show_sidebar && !main_view.collapsed);
		app.settings.set_int (PREF_BROWSER_SIDEBAR_WIDTH, (int) main_view.max_sidebar_width);
		app.settings.set_boolean (PREF_BROWSER_PROPERTIES_VISIBLE, content_view.show_sidebar && !content_view.collapsed);
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

		if ((file_sorter.name != null) && (file_sorter.name != "Private::Unsorted")) {
			app.settings.set_string (PREF_BROWSER_SORT_TYPE, app.migration.test.get_old_key (file_sorter.name));
			app.settings.set_boolean (PREF_BROWSER_SORT_INVERSE, file_sorter.inverse);
		}

		if ((folder_tree.sort.name != null) && (folder_tree.sort.name != "Private::Unsorted")) {
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

	void edit_file () {
		try {
			var position = get_selected_position ();
			if (position == uint.MAX) {
				throw new IOError.FAILED (_("No file selected"));
			}
			var file = file_grid.model.get_item (position) as FileData;
			if (file == null) {
				throw new IOError.FAILED (_("No file selected"));
			}
			view_position_async.begin (position, ViewFlags.DEFAULT, (_obj, res) => {
				if (view_position_async.end (res)) {
					window.viewer.show_editor_tools (true);
				}
			});
		}
		catch (Error error) {
			window.show_error (error);
		}
	}

	public async bool view_position_async (uint position, ViewFlags flags = ViewFlags.DEFAULT) {
		var file = file_grid.model.get_item (position) as FileData;
		if (file == null) {
			return false;
		}
		window.viewer.position = (int) position;
		yield window.viewer.view_file_async (file, flags | ViewFlags.FOCUS | ViewFlags.DONT_UPDATE_POSITION);
		return true;
	}

	public bool view_position (uint position, ViewFlags flags = ViewFlags.DEFAULT) {
		var file = file_grid.model.get_item (position) as FileData;
		if (file == null) {
			return false;
		}
		window.viewer.position = (int) position;
		window.viewer.view_file (file, flags | ViewFlags.DONT_UPDATE_POSITION);
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

	public bool view_last_file () {
		var pos = file_grid.model.get_n_items () - 1;
		if ((window.viewer.position == pos) || !view_position (pos)) {
			window.edge_reached ();
			return false;
		}
		return true;
	}

	public bool view_first_file () {
		if ((window.viewer.position == 0) || !view_position (0)) {
			window.edge_reached ();
			return false;
		}
		return true;
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
		file_filter.after_adding_files ();
		if (!file_filter.filter.match (file_data)) {
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

	public void files_added (File parent, GenericList<File> files) {
		if (!folder_tree.current_folder.file.equal (parent)) {
			return;
		}
		var new_files = new GenericList<File>();
		foreach (unowned var file in files) {
			stdout.printf ("> BROWSER: FILE ADDED: %s\n", file.get_uri ());
			var iter = folder_tree.current_children.iterator ();
			var pos = iter.find_first ((file_data) => file_data.file.equal (file));
			if (pos < 0) {
				stdout.printf ("> BROWSER: APPEND\n");
				new_files.model.append (file);
			}
			else {
				file_changed (file);
			}
		}
		if (!new_files.is_empty ()) {
			add_files.begin (new_files);
		}
	}

	async void update_file (FileData file_data) {
		var attributes = get_list_attributes ();
		var local_job = window.new_job (_("Updating %s").printf (file_data.get_display_name ()),
			JobFlags.FOREGROUND,
			"folder-symbolic");
		try {
			var info = yield Files.query_info (file_data.file, attributes, local_job.cancellable);
			file_data.update_info (info);
			file_data.remove_thumbnail ();
			file_filter.after_adding_files ();
			stdout.printf ("> BROWSER: UPDATE FILE [1]\n");
			if (file_filter.filter.match (file_data)) {
				stdout.printf ("> BROWSER: UPDATE FILE [2]\n");
				thumbnailer.queue_load_next ();
				thumbnailer.add (file_data);
			}
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
			update_total_files ();
		}
	}

	public void file_changed (File file) {
		stdout.printf ("> BROWSER: FILE CHANGED: %s\n", file.get_uri ());
		var iter = folder_tree.current_children.iterator ();
		var file_data = iter.find_first_item ((file_data) => file_data.file.equal (file));
		if (file_data != null) {
			stdout.printf ("> BROWSER: UPDATE FILE\n");
			update_file.begin (file_data);
		}
	}

	public void files_removed (File parent, GenericList<File> files) {
		if (!folder_tree.current_folder.file.equal (parent)) {
			return;
		}
		files_deleted (files);
	}

	public void files_deleted (GenericList<File> files) {
		var removed_files = 0;
		foreach (unowned var file in files) {
			stdout.printf ("> BROWSER: FILE DELETED: %s\n", file.get_uri ());

			// If the current folder was deleted get its position inside
			// the folder tree.
			var tree_pos = -1;
			if ((folder_tree.current_folder != null)
				&& (folder_tree.current_folder.file.equal (file)
					|| folder_tree.current_folder.file.has_prefix (file)))
			{
				folder_tree.get_file_row (file, out tree_pos);
			}

			// Remove from the folder tree.
			var parent = file.get_parent ();
			var parent_row = folder_tree.get_file_row (parent);
			if (parent_row != null) {
				var parent_data = parent_row.item as Gth.FileData;
				if (parent_data != null) {
					var children = parent_data.get_children_model ();
					var iter = children.iterator ();
					var file_pos = iter.find_first ((file_data) => file_data.file.equal (file));
					if (file_pos >= 0) {
						children.model.remove ((uint) file_pos);
					}
				}
			}

			// The current folder was deleted.
			if (tree_pos >= 0) {
				// Load the next folder in the folder tree.
				var next_folder = folder_tree.get_file_at_position (tree_pos);
				if (next_folder != null) {
					open_location (next_folder, LoadAction.OPEN_SUBFOLDER);
					return;
				}

				// Load the first existing parent.
				while ((parent != null) && !parent.query_exists ()) {
					parent = parent.get_parent ();
				}
				if (parent == null) {
					parent = Files.get_home ();
				}
				open_location (parent, LoadAction.OPEN_SUBFOLDER);
				return;
			}

			// Remove files inside the current folder.
			var iter = folder_tree.current_children.iterator ();
			var pos = iter.find_first ((file_data) => file_data.file.equal (file));
			if (pos >= 0) {
				folder_tree.current_children.model.remove ((uint) pos);
				removed_files++;
			}

			// Update the property sidebar.
			if (property_sidebar.current_file != null) {
				if (file.equal (property_sidebar.current_file.file)) {
					property_sidebar.current_file = null;
				}
			}
		}

		if (removed_files > 0) {
			file_filter.after_removing_files ();
		}
		update_total_files ();
		update_selection_info ();
	}

	async void paste_files_from_clipboard (File destination, Job job) throws Error {
		unowned var clipboard = get_clipboard ();
		unowned string mime_type;
		var stream = yield clipboard.read_async (
			{ "text/uri-list", "gthumb/cut-files" },
			Priority.DEFAULT,
			job.cancellable,
			out mime_type);
		var bytes = yield Files.read_all_async (stream, job.cancellable);
		unowned var data_as_string = (string) bytes.get_data ();
		var text = data_as_string.substring (0, bytes.length);
		//stdout.printf ("> text: '%s'\n", text);
		//stdout.printf ("> type: '%s'\n", mime_type);
		var uris = text.split_set ("\n\r");
		var files = new GenericList<File> ();
		foreach (unowned var uri in uris) {
			//stdout.printf ("> URI: %s\n", uri);
			files.model.append (File.new_for_uri (uri));
		}
		var source = app.get_source_for_file (destination);
		if (mime_type == "gthumb/cut-files") {
			if (!(source is FileSourceVfs)) {
				throw new IOError.FAILED (_("Cannot move files here"));
			}
			yield window.file_manager.move_files (files, destination, job);
		}
		else {
			yield source.add_files (window, destination, files, job);
		}
	}

	public void metadata_changed (FileData file_data) {
		if ((property_sidebar.current_file != null)
			&& property_sidebar.current_file.file.equal (file_data.file))
		{
			property_sidebar.current_file = file_data;
		}
		var iter = folder_tree.current_children.iterator ();
		while (iter.next ()) {
			var _file_data = iter.get ();
			if (_file_data.file.equal (file_data.file)) {
				_file_data.update_info (file_data.info);
				break;
			}
		}
	}

	async void new_folder () {
		var local_job = window.new_job ("New Folder");
		try {
			var read_filename = new ReadFilename (_("New Folder"), _("_Create"));
			var basename = yield read_filename.read_value (window, local_job);
			var folder = folder_tree.context_file.file.get_child_for_display_name (basename);
			yield Files.make_directory_async (folder, local_job.cancellable);
			yield open_location_async (folder, LoadAction.OPEN_NEW_FOLDER);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	async void rename_folder () {
		var local_job = window.new_job ("Rename Folder");
		try {
			var old_folder = folder_tree.context_file.file;
			var parent = old_folder.get_parent ();
			var read_filename = new ReadFilename (_("Rename"), _("Rename"));
			read_filename.folder = parent;
			read_filename.check_exists = true;
			read_filename.default_value = folder_tree.context_file.info.get_edit_name ();
			var basename = yield read_filename.read_value (window, local_job);
			var new_folder = parent.get_child_for_display_name (basename);
			yield old_folder.move_async (new_folder, FileCopyFlags.NONE, Priority.DEFAULT, local_job.cancellable, null);
			// TODO: Monitor.renamed (old_folder, new_folder);
			yield open_location_async (new_folder);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	File get_context_library () {
		File parent;
		if (folder_tree.context_file.info.get_attribute_boolean ("gthumb::no-child")) {
			parent = folder_tree.context_file.file.get_parent ();
		}
		else {
			parent = folder_tree.context_file.file;
		}
		return parent;
	}

	async void new_catalog () {
		var local_job = window.new_job ("New Catalog");
		try {
			var read_filename = new ReadFilename (_("New Catalog"), _("_Create"));
			var basename = yield read_filename.read_value (window, local_job);
			var library = get_context_library ();
			var catalog = new Catalog ();
			catalog.name = basename;
			catalog.file = library.get_child_for_display_name (basename + ".catalog");
			yield catalog.save_async (local_job.cancellable);
			yield open_location_async (catalog.file);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	async void new_library () {
		var local_job = window.new_job ("New Library");
		try {
			var read_filename = new ReadFilename (_("New Library"), _("_Create"));
			var basename = yield read_filename.read_value (window, local_job);
			var library = get_context_library ();
			var folder = library.get_child_for_display_name (basename);
			var gio_folder = Catalog.to_gio_file (folder);
			yield Files.make_directory_async (gio_folder, local_job.cancellable);
			yield open_location_async (folder);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	async void rename_catalog () {
		var local_job = window.new_job ("Rename Catalog");
		try {
			var old_file = folder_tree.context_file.file;
			var old_gio_file = Catalog.to_gio_file (old_file);
			var read_filename = new ReadFilename (_("Rename"), _("_Rename"));
			read_filename.folder = old_gio_file.get_parent ();
			read_filename.check_exists = true;
			read_filename.default_value = folder_tree.context_file.info.get_edit_name ();
			var basename = yield read_filename.read_value (window, local_job);
			File new_file;
			if (folder_tree.context_file.get_content_type () == "gthumb/library") {
				new_file = old_file.get_parent ().get_child_for_display_name (basename);
				var new_gio_file = Catalog.to_gio_file (new_file);
				yield old_gio_file.move_async (new_gio_file, FileCopyFlags.NONE, Priority.DEFAULT, local_job.cancellable, null);
			}
			else {
				var data = yield Files.load_contents_async (old_gio_file, local_job.cancellable);
				var catalog = Catalog.new_from_data (old_file, data);
				catalog.name = basename;
				yield catalog.save_async (local_job.cancellable);
				new_file = catalog.file;
			}
			yield open_location_async (new_file);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	async void delete_catalog () {
		var local_job = window.new_job ("Delete Catalog");
		try {
			var dialog = new Adw.AlertDialog (_("Delete Permanently?"), null);
			dialog.body_use_markup = true;
			dialog.body = _("This action is not reversable. Do you want to delete <i>%s</i>?").printf (
				Markup.escape_text (folder_tree.context_file.get_display_name ()));
			dialog.add_responses (
				"cancel",  _("_Cancel"),
				"delete", _("_Delete")
			);
			dialog.set_response_appearance ("delete", Adw.ResponseAppearance.DESTRUCTIVE);
			dialog.default_response = "cancel";
			dialog.close_response = "cancel";
			local_job.opens_dialog ();
			var response = yield dialog.choose (window, local_job.cancellable);
			local_job.dialog_closed ();
			if (response == "cancel") {
				throw new IOError.CANCELLED ("Cancelled");
			}
			var gio_file = Catalog.to_gio_file (folder_tree.context_file.file);
			yield gio_file.delete_async (Priority.DEFAULT, local_job.cancellable);
			app.monitor.file_deleted (folder_tree.context_file.file);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	async void move_catalog () {
		var local_job = window.new_job ("Move Catalog");
		try {
			var selector = new Gth.FolderSelector ();
			selector.root = Catalog.get_root ();
			selector.for_moving = true;
			var catalog = folder_tree.context_file.file;
			var parent = catalog.get_parent ();
			local_job.opens_dialog ();
			var destination = yield selector.select_folder (window, parent, local_job.cancellable);
			if (destination.equal (parent)) {
				throw new IOError.FAILED (_("Source and destination are the same"));
			}
			var new_catalog = destination.get_child (catalog.get_basename ());
			var gio_source = Catalog.to_gio_file (catalog);
			var gio_destination = Catalog.to_gio_file (new_catalog);
			yield gio_source.move_async (gio_destination,
				FileCopyFlags.ALL_METADATA, Priority.DEFAULT,
				local_job.cancellable, null);
			app.monitor.file_created (new_catalog);
			app.monitor.file_deleted (catalog);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.dialog_closed ();
			local_job.done ();
		}
	}

	public void focus_thumbnail_list () {
		if (total_files > 0) {
			file_grid.grab_focus ();
		}
	}

	async void ask_location () {
		var local_job = window.new_job ("Open Location");
		try {
			var dialog = new Gtk.FileDialog ();
			dialog.modal = true;
			dialog.title = _("Open");
			dialog.initial_folder = window.get_current_vfs_folder ();
			local_job.opens_dialog ();
			var location = yield dialog.select_folder (window, local_job.cancellable);
			open_location (location);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.dialog_closed ();
			local_job.done ();
		}
	}

	async void add_to_catalog () {
		var files = get_selected_files ();
		if (files.is_empty ()) {
			return;
		}
		var local_job = window.new_job ("Add to Catalog");
		try {
			var selector = new Gth.FolderSelector ();
			selector.root = Catalog.get_root ();
			var last_catalog = Settings.get_file (app.settings, PREF_BROWSER_LAST_CATALOG);
			local_job.opens_dialog ();
			var catalog_file = yield selector.select_folder (window, last_catalog, local_job.cancellable);
			yield Catalog.add_files (catalog_file, files, local_job);
			app.settings.set_string (PREF_BROWSER_LAST_CATALOG, catalog_file.get_uri ());
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.dialog_closed ();
			local_job.done ();
		}
	}

	void remove_from_catalog () {
		var files = get_selected_files ();
		if ((files == null) || files.is_empty ()) {
			return;
		}
		window.remove_files (files);
	}

	enum DropSide {
		NONE,
		LEFT,
		RIGHT;
	}

	Gtk.ListItem? get_item_at (double x, double y, out DropSide side) {
		Graphene.Point p = { (float) x, (float) y };
		window.compute_point (file_grid, p, out p);
		var top = 0;
		var bottom = file_grid.vadjustment.get_page_size ();
		Gtk.ListItem closest_item = null;
		float min_distance = 0f;
		side = DropSide.NONE;
		foreach (unowned Gtk.ListItem item in binded_grid_items) {
			if (!item.child.get_mapped ()) {
				continue;
			}
			Graphene.Rect bounds;
			if (item.child.compute_bounds (file_grid, out bounds)) {
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
				// 	p.x, p.y);

				if ((p.y < bounds.origin.y) || (p.y > bounds.origin.y + bounds.size.height)) {
					continue;
				}

				if ((p.x >= bounds.origin.x) && (p.x <= bounds.origin.x + bounds.size.width)) {
					side = (p.x < bounds.origin.x + (bounds.size.width / 2)) ? DropSide.LEFT : DropSide.RIGHT;
					return item;
				}

				if (p.x < bounds.origin.x) {
					var distance = bounds.origin.x - p.x;
					if ((closest_item == null) || (distance < min_distance)) {
						closest_item = item;
						min_distance = distance;
						side = DropSide.LEFT;
					}
				}
				else {
					var distance = p.x - (bounds.origin.x + bounds.size.width);
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

	void activate_reordering (bool value) {
		reordering = value;
		if (reordering) {
			add_controller (drop_target);
			file_grid.enable_rubberband = false;
		}
		else {
			remove_controller (drop_target);
			file_grid.enable_rubberband = true;
		}
		Util.set_active (window.action_group, "reorder-files", reordering);
	}

	void reorder_selected_file (uint destination) {
		var selection = file_grid.model.get_selection ();
		var n_selected = (uint) selection.get_size ();
		if (n_selected == 0) {
			return;
		}
		//stdout.printf ("> MOVE SELECTION TO %u\n", destination);

		// Create the new file list.

		var new_list = new GenericList<FileData> ();
		var visible = new GenericSet<File> (Util.file_hash, Util.file_equal);

		// Add the visible files to the list.

		var iter = visible_files.iterator ();
		while (iter.next ()) {
			var file_data = iter.get ();
			new_list.model.append (file_data);
			visible.add (file_data.file);
		}

		var selected = new GenericList<FileData> ();
		for (var i = 0; i < n_selected; i++) {
			var old_pos = selection.get_nth (i);
			var selected_file = file_grid.model.get_item (old_pos) as FileData;
			selected.model.append (selected_file);

			// Remove the selected file from the new list.
			iter = new_list.iterator ();
			while (iter.next ()) {
				if (iter.get () == selected_file) {
					var pos = iter.index ();
					if (pos < destination) {
						destination--;
					}
					new_list.model.remove (pos);
				}
			}
		}

		// Add the selected files at destination.
		foreach (var file in selected) {
			new_list.model.insert (destination, file);
			destination++;
		}

		// Update the visible files position.
		var pos = 0;
		iter = new_list.iterator ();
		while (iter.next ()) {
			var file_data = iter.get ();
			file_data.set_position (pos++);
		}

		// Add the hidden files at the end.
		foreach (var file_data in folder_tree.current_children) {
			if (!visible.contains (file_data.file)) {
				new_list.model.append (file_data);
				file_data.set_position (pos++);
			}
		}

		var files = new GenericList<File> ();
		iter = new_list.iterator ();
		while (iter.next ()) {
			var file_data = iter.get ();
			files.model.append (file_data.file);
			//stdout.printf ("> %s (pos: %u)\n", file_data.get_display_name (), file_data.get_position ());
		}

		var local_job = window.new_job (_("Saving"), JobFlags.FOREGROUND);
		folder_tree.current_source.save_order.begin (window, folder_tree.current_folder.file, files, local_job, (_obj, res) => {
			try {
				folder_tree.current_source.save_order.end (res);
			}
			catch (Error error) {
				window.show_error (error);
			}
			finally {
				local_job.done ();
				folder_tree.current_folder.info.set_attribute_string ("sort::type", "Private::Unsorted");
				folder_tree.current_folder.info.set_attribute_boolean ("sort::inverse", false);
				file_sorter.set_order ("Private::Unsorted", false);
			}
		});
	}

	void add_to_selection (uint number) {
		var files = get_selected_files ();
		if (files.is_empty ()) {
			window.show_error (new IOError.FAILED (_("No file selected")));
			return;
		}
		var selection = app.selections.get_selection (number);
		selection.add_files (files);
	}

	void remove_from_selection (uint number) {
		var files = get_selected_files ();
		if (files.is_empty ()) {
			window.show_error (new IOError.FAILED (_("No file selected")));
			return;
		}
		var selection = app.selections.get_selection (number);
		selection.remove_files (files);
	}

	WindowState state = null;

	void open_selection (uint number) {
		var uri = "selection:///%u".printf (number);
		var location = File.new_for_uri (uri);
		if (folder_tree.current_folder.file.equal (location)) {
			if (state != null) {
				window.restore_state.begin (state);
				return;
			}
		}
		state = new WindowState (window);
		open_location (location);
	}

	void open_terminal (File file) {
		try {
			var files = new List<File> ();
			files.append (file);
			var context = window.display.get_app_launch_context ();
			context.set_timestamp (0);
			var settings = new GLib.Settings (GTHUMB_TERMINAL_SCHEMA);
			var info = AppInfo.create_from_commandline (settings.get_string (PREF_TERMINAL_COMMAND), _("Terminal"), AppInfoCreateFlags.NONE);
			info.launch (files, context);
		}
		catch (Error error) {
			window.show_error (error);
		}
	}

	void open_file_manager (File file) {
		var launcher = new Gtk.FileLauncher (file);
		var local_job = window.new_job ("Open %s".printf (file.get_uri ()));
		launcher.launch.begin (window, local_job.cancellable, (_obj, res) => {
			try {
				launcher.launch.end (res);
			}
			catch (Error error) {
				window.show_error (error);
			}
			finally {
				local_job.done ();
			}
		});
	}

	[GtkChild] unowned Adw.OverlaySplitView main_view;
	[GtkChild] public unowned Adw.OverlaySplitView content_view;
	[GtkChild] public unowned Gth.FilterBar filter_bar;
	[GtkChild] public unowned Gtk.MenuButton app_menu_button;
	[GtkChild] public unowned Gtk.MenuButton scripts_menu_button;
	[GtkChild] unowned Gth.ActionPopover bookmark_popover;
	[GtkChild] unowned Gth.ActionPopover history_popover;
	[GtkChild] public unowned Gtk.GridView file_grid;
	[GtkChild] unowned Gtk.Stack folder_stack;
	[GtkChild] unowned Gtk.Widget non_empty_folder;
	[GtkChild] public unowned Adw.StatusPage empty_folder;
	[GtkChild] unowned Gtk.ToggleButton vfs_button;
	[GtkChild] unowned Gtk.ToggleButton catalog_button;
	[GtkChild] unowned Gtk.ToggleButton selection_button;
	[GtkChild] unowned Gtk.Stack sidebar_stack;
	//[GtkChild] unowned Gtk.Stack second_sidebar_stack;
	[GtkChild] public unowned Gth.PropertySidebar property_sidebar;
	[GtkChild] unowned Gtk.MenuButton location_button;
	[GtkChild] unowned Gtk.Label location_label;
	[GtkChild] unowned Gth.ActionList location_actions;
	[GtkChild] unowned Gth.SidebarResizer sidebar_resizer;
	[GtkChild] unowned Gtk.Button edit_catalog_button;
	[GtkChild] unowned Gtk.Button update_search_button;
	[GtkChild] unowned Gtk.Button update_folder_button;
	[GtkChild] unowned Gtk.Button reorder_button;
	[GtkChild] unowned Gtk.PopoverMenu file_context_menu;
	[GtkChild] unowned Gtk.PopoverMenu context_menu;
	[GtkChild] public unowned Gth.FolderStatus folder_status;
	[GtkChild] public unowned Gth.ActionPopover tools_popover;
	[GtkChild] public unowned Gtk.ToggleButton view_properties;

	Gth.Test general_filter;
	Gth.Thumbnailer thumbnailer;
	Gth.History history;
	weak Window _window;
	Queue<File> current_parents;
	File last_folder = null;
	File last_catalog = null;
	File last_selection = null;
	GenericArray<Gtk.ListItem> binded_grid_items;
	ActionCategory actions_category;
	ActionCategory bookmarks_category;
	ActionCategory parents_category;
	uint total_files = 0;
	uint64 total_size = 0;
	Gtk.DropTarget drop_target;
}

public class Gth.FileSorter : Gtk.Sorter {
	public string name;
	public bool inverse;
	public weak Gth.SortInfo? sort_info;
	public Gth.Sort last;

	public FileSorter (Browser _browser) {
		name = null;
		inverse = false;
		sort_info = null;
		browser = _browser;
		last = { null, false };
	}

	public void set_order (string _name, bool _inverse) {
		name = _name;
		inverse = _inverse;
		sort_info = app.get_sorter_by_id (name);
		if (sort_info == null) {
			sort_info = app.get_sorter_by_id ("Private::Unsorted");
		}
		if ((name != null) && (name != "Private::Unsorted")) {
			last = { name, inverse };
		}
		changed (Gtk.SorterChange.DIFFERENT);
	}

	public void reset () {
		if (browser.folder_tree.current_folder != null) {
			set_order (
				browser.folder_tree.current_folder.get_sort_name (last.name),
				browser.folder_tree.current_folder.get_inverse_order (last.inverse));
		}
		else {
			set_order (last.name, last.inverse);
		}
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
		filter = new Gth.Filter ();
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

	public void reset (Gtk.FilterChange filter_change = Gtk.FilterChange.DIFFERENT) {
		visible.remove_all ();
		var iter = filter.iterator (browser.folder_tree.current_children);
		while (iter.next ()) {
			unowned var file_data = iter.get ();
			visible.add (file_data.file);
		}
		changed (filter_change);
	}

	public void after_adding_files () {
		if (filter.limit_type == Filter.LimitType.NONE) {
			reset (Gtk.FilterChange.LESS_STRICT);
		}
		else {
			reset ();
		}
	}

	public void after_removing_files () {
		if (filter.limit_type == Filter.LimitType.NONE) {
			reset (Gtk.FilterChange.MORE_STRICT);
		}
		else {
			reset ();
		}
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
