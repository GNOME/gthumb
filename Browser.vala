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

	public GenericList<FileData> visible_files;
	public Gth.Sort sort = { null, false };
	public bool fast_file_type = false;
	public bool show_hidden_files = false;
	SidebarState sidebar_state = SidebarState.NONE;
	public int thumbnail_size;
	[GtkChild] public unowned Gth.FolderTree folder_tree;
	[GtkChild] public unowned Gth.Status status;

	construct {
		visible_files = new GenericList<FileData>();
		thumbnailer = new Thumbnailer (get_next_file_for_thumbnailer);
		history = new History (this);
		current_parents = null;
		binded_grid_items = new GenericArray<Gtk.ListItem> ();
		actions_category = new ActionCategory ("", -1);
		bookmarks_category = new ActionCategory (_("Bookmarks"), 1);
		parents_category = new ActionCategory (_("Path"), 1);
	}

	void init () {
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
		var width = app.settings.get_int (PREF_BROWSER_WINDOW_WIDTH);
		var height = app.settings.get_int (PREF_BROWSER_WINDOW_HEIGHT);
		window.set_default_size (width, height);
		if (app.settings.get_boolean (PREF_BROWSER_WINDOW_MAXIMIZED)) {
			window.maximize ();
		}
		main_view.max_sidebar_width = (double) app.settings.get_int (PREF_BROWSER_SIDEBAR_WIDTH);
		content_view.max_sidebar_width = (double) app.settings.get_int (PREF_BROWSER_PROPERTIES_WIDTH);

		// Restore settings.
		sort = {
			app.settings.get_string (PREF_BROWSER_SORT_TYPE),
			app.settings.get_boolean (PREF_BROWSER_SORT_INVERSE)
		};
		general_filter = app.get_general_filter ();
		active_filter = app.get_last_active_filter ();
		fast_file_type = app.settings.get_boolean (PREF_BROWSER_FAST_FILE_TYPE);
		show_hidden_files = app.settings.get_boolean (PREF_BROWSER_SHOW_HIDDEN_FILES);
		thumbnail_size = app.settings.get_int (PREF_BROWSER_THUMBNAIL_SIZE);
		folder_tree.sort = {
			app.settings.get_string (PREF_BROWSER_FOLDER_TREE_SORT_TYPE),
			app.settings.get_boolean (PREF_BROWSER_FOLDER_TREE_SORT_INVERSE)
		};
		folder_tree.show_hidden = show_hidden_files;
		content_view.show_sidebar = app.settings.get_boolean (PREF_BROWSER_PROPERTIES_VISIBLE);

		init_actions ();
	}

	async void load_folder (File location, LoadAction load_action) throws Error {
		thumbnailer.cancel ();
		folder_tree.list_attributes = get_list_attributes (true);
		yield folder_tree.load_folder (location, load_action);
		update_thumbnail_list ();
		update_title ();
		update_load_sensitivity ();
		update_location_commands ();
		if (folder_tree.current_folder.file.has_uri_scheme ("catalog")) {
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

	public void open_location (File location, LoadAction load_action = LoadAction.OPEN, File? file_to_select = null) {
		window.set_page (Window.Page.BROWSER);
		if (load_action.changes_current_folder ()) {
			set_sidebar_state (location.has_uri_scheme ("catalog") ? SidebarState.CATALOGS : SidebarState.FILES);
		}
		load_folder.begin (location, load_action, (_obj, res) => {
			try {
				load_folder.end (res);
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

	public void first_load (File location, File? file_to_select) {
		filter_bar.set_active_filter (active_filter);
		if (app.one_window ()) {
			// Restore the last saved history.
			history.restore_from_file ();
		}
		else {
			// Copy the previously focused window history.
			unowned var windows = app.get_windows ();
			foreach (unowned var w in windows) {
				if ((w != window) && (w is Gth.Window)) {
					var previous_window = w as Gth.Window;
					history.copy (previous_window.browser.history);
					break;
				}
			}
		}
		app.bookmarks.load_from_file.begin ((_obj, res) => {
			app.bookmarks.load_from_file.end (res);
			update_bookmarks_menu ();
			open_location (location, LoadAction.OPEN, file_to_select);
		});
	}

	public void update_sort_order (string name, bool inverse) {
		sort = { name, inverse };
		update_thumbnail_list ();
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
		}
	}

	void set_show_hidden (bool show) {
		show_hidden_files = show;
		folder_tree.show_hidden = show_hidden_files;
		update_thumbnail_list ();
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
		if (folder_tree.current_folder.file.has_uri_scheme ("catalog")) {
			var uri = folder_tree.current_folder.file.get_uri ();
			is_catalog = uri.has_suffix (".catalog");
			is_search = uri.has_suffix (".search");
		}
		edit_catalog_button.visible = is_catalog || is_search;
		update_search_button.visible = is_search;
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
		if (sort.name != null) {
			var sort_info = app.get_sorter_by_id (sort.name);
			if (sort_info != null) {
				if (!Strings.empty (sort_info.required_attributes)) {
					attributes.append (",");
					attributes.append (sort_info.required_attributes);
				}
			}
		}

		// Attributes required for the thumbnail caption.
		var thumbnail_caption = app.settings.get_string (PREF_BROWSER_THUMBNAIL_CAPTION);
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

	unowned Gth.Filter get_file_filter (bool recalc = false) {
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

	public void update_title () {
		if (folder_tree.current_folder != null) {
			window.title = folder_tree.current_folder.info.get_display_name ();
		}
		else {
			window.title = "Thumbnails";
		}
		location_button.label = window.title;
	}

	void update_thumbnail_list () {
		// Filter the files.
		var visible_children = new GenericList<FileData>();
		var filter = get_file_filter (true);
		var iter = filter.iterator (folder_tree.current_children);
		while (iter.next ()) {
			visible_children.model.append (iter.get ());
		}

		// Sort the files.
		Gth.Sort local_sort = {
			folder_tree.current_folder.get_sort_name (sort.name),
			folder_tree.current_folder.get_inverse_order (sort.inverse)
		};
		if ((local_sort.name != filter.sort.name) || (local_sort.inverse != filter.sort.inverse)) {
			unowned var sort_info = app.get_sorter_by_id (local_sort.name);
			if (sort_info == null) {
				sort_info = app.get_sorter_by_id ("file::name");
			}
			if (sort_info.cmp_func != null) {
				visible_children.model.sort ((a, b) => {
					var result = sort_info.cmp_func ((FileData) a, (FileData) b);
					if (local_sort.inverse)
						result *= -1;
					return result;
				});
			}
		}

		// Update the view model.
		var grid_model = file_grid.model;
		file_grid.model = null;
		visible_files.model.remove_all ();
		uint total_files = 0;
		uint64 total_size = 0;
		foreach (unowned var file in visible_children) {
			visible_files.model.append (file);
			total_files++;
			total_size += file.info.get_size ();
		}
		file_grid.model = grid_model;

		status.set_list_info (total_files, total_size);
		window.viewer.set_list_info (total_files);
		folder_stack.set_visible_child ((total_files == 0) ? empty_folder : non_empty_folder);

		status.set_selection_info (0, 0);
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

	public File? get_selected_file () {
		var selected = file_grid.model.get_selection ();
		if (selected.get_size () != 1)
			return null;
		var pos = selected.get_nth (0);
		var file_data = file_grid.model.get_item (pos) as FileData;
		return file_data?.file;
	}

	bool is_selected (File file) {
		var selected = file_grid.model.get_selection ();
		for (int64 idx = 0; idx < selected.get_size (); idx++) {
			var pos = selected.get_nth ((uint) idx);
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

	GenericArray<Gth.FileData> get_selected () {
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
		status.set_selection_info (total_files, total_size);
		if (total_files == 1) {
			var local_job = window.new_job ("Load metadata for %s".printf (selected_file.file.get_uri ()));
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
			property_sidebar.current_file = null;
		}
	}

	void init_actions () {
		var builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/app-menu.ui");
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

		action = new SimpleAction.stateful ("set-sort-name", GLib.VariantType.STRING, new Variant.string (sort.name ?? ""));
		action.activate.connect ((_action, param) => {
			_action.set_state (param);
			// TODO
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("set-inverse-order", GLib.VariantType.BOOLEAN, new Variant.boolean (sort.inverse));
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
			var window = new Gth.Window (app, folder_tree.current_folder.file, null);
			window.present ();
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
		action.activate.connect ((_action, param) => {
			open_home ();
		});
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
			var dialog = new Gth.SearchDialog (folder_tree.current_folder.file);
			dialog.transient_for = window;
			dialog.present ();
			dialog.focus_first_rule ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("edit-catalog", null);
		action.activate.connect ((_action, param) => {
			edit_catalog.begin ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("update-search", null);
		action.activate.connect ((_action, param) => {
			// TODO
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-with", null);
		action.activate.connect ((_action, param) => {
			open_selected_files.begin ();
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
		action.activate.connect ((_action, param) => {
			view_previous_file ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("view-next", null);
		action.activate.connect ((_action, param) => {
			view_next_file ();
		});
		action_group.add_action (action);
	}

	void init_folder_tree () {
		folder_tree.load.connect ((location, action) => {
			load_folder (location, action);
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
			list_item.child = new Gth.FileListItem (this, thumbnail_size, thumbnail_attributes_v);
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
	}

	Gth.FileData? get_next_file_for_thumbnailer () {
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
				if (file_data.thumbnail_state != ThumbnailState.ICON)
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

	async void edit_catalog () {
		var local_job = window.new_job ("Edit catalog %s".printf (folder_tree.current_folder.file.get_uri ()));
		try {
			var editor = new Gth.CatalogEditor ();
			var catalog = yield editor.edit_catalog (window, folder_tree.current_folder.file, local_job.cancellable);
			yield catalog.save_async (local_job.cancellable);

			// Update the current folder info
			folder_tree.current_folder.set_file (catalog.file);
			catalog.update_file_info (folder_tree.current_folder.info);
			folder_tree.current_folder.info_changed ();

			// TODO: start searching if the search parameters changed
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	async void open_selected_files () {
		var local_job = window.new_job ("Choose application");
		try {
			var files = get_selected ();
			if (files.length == 0) {
				throw new IOError.FAILED (_("No file selected"));
			}
			var app_selector = new Gth.AppSelector ();
			var app_info = yield app_selector.select_app (window, files, local_job.cancellable);
			var uris = new List<string>();
			foreach (unowned var file_data in files)
				uris.append (file_data.file.get_uri ());
			var context = get_display ().get_app_launch_context ();
			context.set_timestamp (0);
			context.set_icon (app_info.get_icon ());
			app_info.launch_uris (uris, context);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
		}
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
			app.settings.set_int (PREF_BROWSER_PROPERTIES_WIDTH, (int) content_view.max_sidebar_width);
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

		if (sort.name != null) {
			app.settings.set_string (PREF_BROWSER_SORT_TYPE, sort.name);
			app.settings.set_boolean (PREF_BROWSER_SORT_INVERSE, sort.inverse);
		}

		if (folder_tree.sort.name != null) {
			app.settings.set_string (PREF_BROWSER_FOLDER_TREE_SORT_TYPE, folder_tree.sort.name);
			app.settings.set_boolean (PREF_BROWSER_FOLDER_TREE_SORT_INVERSE, folder_tree.sort.inverse);
		}

		app.save_active_filter (filter_bar.filter);
		history.save_to_file ();
	}

	[GtkCallback]
	void on_file_activate (uint position) {
		view_position (position);
	}

	public bool view_position (uint position) {
		var file = file_grid.model.get_item (position) as FileData;
		if (file == null) {
			return false;
		}
		window.viewer.set_file_position (position);
		window.viewer.view_file (file);
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

	[GtkChild] unowned Adw.OverlaySplitView main_view;
	[GtkChild] public unowned Adw.OverlaySplitView content_view;
	[GtkChild] unowned Gth.FilterBar filter_bar;
	[GtkChild] unowned Gtk.MenuButton app_menu_button;
	[GtkChild] unowned Gth.ActionPopover bookmark_popover;
	[GtkChild] unowned Gth.ActionPopover history_popover;
	[GtkChild] unowned Gtk.GridView file_grid;
	[GtkChild] unowned Gtk.Stack folder_stack;
	[GtkChild] unowned Gtk.Widget non_empty_folder;
	[GtkChild] unowned Gtk.Widget empty_folder;
	[GtkChild] unowned Gtk.ToggleButton vfs_button;
	[GtkChild] unowned Gtk.ToggleButton catalog_button;
	[GtkChild] public unowned Adw.ToastOverlay toast_overlay;
	[GtkChild] unowned Gtk.Stack sidebar_stack;
	[GtkChild] unowned Gtk.Stack second_sidebar_stack;
	[GtkChild] public unowned Gth.PropertySidebar property_sidebar;
	[GtkChild] unowned Gtk.MenuButton location_button;
	[GtkChild] unowned Gth.ActionList location_actions;
	[GtkChild] unowned Gth.SidebarResizer sidebar_resizer;
	[GtkChild] unowned Gtk.Button edit_catalog_button;
	[GtkChild] unowned Gtk.Button update_search_button;
	[GtkChild] unowned Gtk.PopoverMenu file_context_menu;

	Gth.Test general_filter;
	Gth.Test active_filter;
	Gth.Thumbnailer thumbnailer;
	Gth.History history;
	double initial_sidebar_width;
	double initial_second_sidebar_width;
	weak Window _window;
	Queue<File> current_parents;
	File last_folder = null;
	File last_catalog = null;
	GenericArray<Gtk.ListItem> binded_grid_items;
	ActionCategory actions_category;
	ActionCategory bookmarks_category;
	ActionCategory parents_category;
}
