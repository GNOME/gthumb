[GtkTemplate (ui = "/app/gthumb/gthumb/ui/window.ui")]
public class Gth.Window : Adw.ApplicationWindow {
	public GenericList<FileData> visible_files = null;
	public Gth.Sort sort = { null, false };
	public bool fast_file_type = false;
	public bool show_hidden_files = false;
	public BrowserSidebarState sidebar_state = BrowserSidebarState.NONE;
	public int thumbnail_size;
	public Gth.JobQueue jobs;

	Queue<File> current_parents;
	File last_folder = null;
	File last_catalog = null;
	GenericArray<Gtk.ListItem> binded_grid_items;
	bool closing = false;

	public enum Page {
		NONE = 0,
		BROWSER,
		VIEWER,
	}

	public Window (Gtk.Application _app, File location, File? file_to_select) {
		Object (application: app);

		named_dialogs = new HashTable<string, Gtk.Window?>(str_hash, str_equal);
		jobs = new Gth.JobQueue ();
		jobs.size_changed.connect (() => {
			if (closing && (jobs.size () == 0)) {
				before_closing ();
				close ();
			}
		});
		visible_files = new GenericList<FileData>();
		thumbnailer = new Thumbnailer (get_next_file_for_thumbnailer);
		history = new WindowHistory (this);
		bookmarks = new WindowBookmarks (this);
		action_group = new SimpleActionGroup ();
		insert_action_group ("win", action_group);
		current_parents = null;
		active_resizer = null;
		binded_grid_items = new GenericArray<Gtk.ListItem> ();

		init_folder_tree ();
		init_file_grid ();

		sidebar_resizer.add_handle (browser_view, Gtk.PackType.END);
		sidebar_resizer.started.connect ((obj) => {
			active_resizer = obj;
		});
		sidebar_resizer.ended.connect (() => {
			active_resizer = null;
		});

		second_sidebar_resizer.add_handle (browser_content_view, Gtk.PackType.START);
		second_sidebar_resizer.started.connect ((obj) => {
			active_resizer = obj;
		});
		second_sidebar_resizer.ended.connect (() => {
			active_resizer = null;
		});

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((x, y) => {
			if (active_resizer != null) {
				active_resizer.update_width (x);
			}
		});
		child.add_controller (motion_events);

		filter_bar.changed.connect (() => update_active_filter ());

		browser_view.notify["show-sidebar"].connect (() => {
			action_group.change_action_state ("show-sidebar", new Variant.boolean (browser_view.show_sidebar));
		});

		// Restore the window size.
		var width = app.settings.get_int (PREF_BROWSER_WINDOW_WIDTH);
		var height = app.settings.get_int (PREF_BROWSER_WINDOW_HEIGHT);
		set_default_size (width, height);
		if (app.settings.get_boolean (PREF_BROWSER_WINDOW_MAXIMIZED)) {
			maximize ();
		}
		browser_view.max_sidebar_width = (double) app.settings.get_int (PREF_BROWSER_SIDEBAR_WIDTH);
		browser_content_view.max_sidebar_width = (double) app.settings.get_int (PREF_BROWSER_PROPERTIES_WIDTH);

		// Browser settings.
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
		browser_content_view.show_sidebar = app.settings.get_boolean (PREF_BROWSER_PROPERTIES_VISIBLE);

		init_actions ();
		set_page (Page.BROWSER);

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

	Gth.Job load_job = null;

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

	public void show_message (string message) {
		browser_toast_overlay.add_toast (new Adw.Toast (message));
	}

	public void show_error (Error error) {
		if (error is IOError.CANCELLED)
			return;
		show_message (error.message);
	}

	public void copy_text (string text) {
		var clipboard = get_clipboard ();
		clipboard.set_text (text);
		show_message ("Copied to Clipboard");
	}

	public void open_location (File location, LoadAction load_action = LoadAction.OPEN, File? file_to_select = null) {
		set_page (Page.BROWSER);
		if (load_action.changes_current_folder ()) {
			set_sidebar_state (location.has_uri_scheme ("catalog") ? BrowserSidebarState.CATALOGS : BrowserSidebarState.FILES);
		}
		load_folder.begin (location, load_action, (_obj, res) => {
			try {
				load_folder.end (res);
				if (file_to_select != null) {
					// TODO
				}
			}
			catch (Error error) {
				show_error (error);
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

	public void update_sort_order (string name, bool inverse) {
		sort = { name, inverse };
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

	public void set_sidebar_state (BrowserSidebarState state) {
		if (state == sidebar_state)
			return;
		sidebar_state = state;
		switch (sidebar_state) {
		case BrowserSidebarState.FILES:
			sidebar_stack.set_visible_child (folder_tree);
			vfs_button.active = true;
			catalog_button.active = false;
			break;
		case BrowserSidebarState.CATALOGS:
			sidebar_stack.set_visible_child (folder_tree);
			vfs_button.active = false;
			catalog_button.active = true;
			break;
		}
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

	public void set_show_hidden (bool show) {
		show_hidden_files = show;
		folder_tree.show_hidden = show_hidden_files;
		update_thumbnail_list ();
	}

	void update_title () {
		if (folder_tree.current_folder != null) {
			title = folder_tree.current_folder.info.get_display_name ();
		}
		else {
			title = "Thumbnails";
		}
	}

	void update_sensitivity () {
		// TODO
	}

	void update_load_sensitivity () {
		File parent = null;
		if (folder_tree.current_folder != null) {
			parent = folder_tree.current_folder.file.get_parent ();
		}
		enable_action ("load-parent", parent != null);
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

	void enable_action (string name, bool enabled) {
		var action = action_group.lookup_action (name) as SimpleAction;
		if (action != null) {
			action.set_enabled (enabled);
		}
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
		var tot_files = 0;
		uint64 tot_size = 0;
		foreach (unowned var file in visible_children) {
			visible_files.model.append (file);
			tot_files++;
			tot_size += file.info.get_size ();
		}
		file_grid.model = grid_model;

		status.set_list_info (tot_files, tot_size);
		folder_stack.set_visible_child ((tot_files == 0) ? empty_folder : non_empty_folder);

		status.set_selection_info (0, 0);
	}

	void update_location_menu () {
		var location_menu = location_button.menu_model as Menu;
		location_menu.remove_all ();
		if (folder_tree.current_folder == null)
			return;
		var location = folder_tree.current_folder.file;
		while (location != null) {
			var item = Util.menu_item_for_file (location);
			item.set_action_and_target_value ("win.load-location", new Variant.string (location.get_uri ()));
			location_menu.append_item (item);
			location = location.get_parent ();
		}
	}

	void update_selection_info () {
		weak Gth.FileData selected_file = null;
		var tot_files = 0;
		uint64 tot_size = 0;
		var selected = file_grid.model.get_selection ();
		for (int64 idx = 0; idx < selected.get_size (); idx++) {
			var pos = selected.get_nth ((uint) idx);
			var file = file_grid.model.get_item (pos) as FileData;
			if (file != null) {
				tot_files++;
				tot_size += file.info.get_size ();
				selected_file = file;
			}
		}
		status.set_selection_info (tot_files, tot_size);
		if (tot_files == 1) {
			var local_job = new_job ("Load metadata for %s".printf (selected_file.file.get_uri ()));
			var source = new FileSourceVfs ();
			source.read_metadata.begin (selected_file.file, "*", local_job.cancellable, (obj, res) => {
				try {
					var selected_file_data = source.read_metadata.end (res);
					property_sidebar.set_file (selected_file_data);
				}
				catch (Error error) {
					stdout.printf ("ERROR: %s\n", error.message);
				}
				local_job.done ();
			});
		}
		else {
			property_sidebar.set_file (null);
		}
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
			var dialog = get_named_dialog ("sort-files");
			if (dialog == null) {
				dialog = new Gth.SortFilesDialog (this);
				set_named_dialog ("sort-files", dialog);
			}
			dialog.present ();
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
			var dialog = get_named_dialog ("sort-folders");
			if (dialog == null) {
				dialog = new Gth.SortFoldersDialog (this);
				set_named_dialog ("sort-folders", dialog);
			}
			dialog.present ();
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

		action = new SimpleAction.stateful ("show-sidebar", null, new Variant.boolean (browser_view.show_sidebar));
		action.activate.connect ((_action, param) => {
			browser_view.show_sidebar = Util.toggle_state (_action);
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("show-second-sidebar", null, new Variant.boolean (browser_content_view.show_sidebar));
		action.activate.connect ((_action, param) => {
			browser_content_view.show_sidebar = Util.toggle_state (_action);
		});
		action_group.add_action (action);

		action = new SimpleAction ("load-home", null);
		action.activate.connect ((_action, param) => {
			open_home ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("show-folders", null);
		action.activate.connect ((_action, param) => {
			if (sidebar_state == BrowserSidebarState.FILES) {
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
			if (sidebar_state == BrowserSidebarState.CATALOGS) {
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
			dialog.transient_for = this;
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

		close_request.connect (() => {
			closing = true;
			if (jobs.size () > 0) {
				jobs.cancel_all ();
				return true;
			}
			before_closing ();
			return false;
		});
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
			list_item.child = new Gth.FileListItem (thumbnail_size, thumbnail_attributes_v);
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
		var local_job = new_job ("Edit catalog %s".printf (folder_tree.current_folder.file.get_uri ()));
		try {
			var editor = new Gth.CatalogEditor ();
			var catalog = yield editor.edit_catalog (this, folder_tree.current_folder.file, local_job.cancellable);
			yield catalog.save_async (local_job.cancellable);

			// Update the current folder info
			folder_tree.current_folder.set_file (catalog.file);
			catalog.update_file_info (folder_tree.current_folder.info);
			folder_tree.current_folder.info_changed ();

			// TODO: start searching if the search parameters changed

			local_job.done ();
		}
		catch (Error error) {
			local_job.done ();
			show_error (error);
		}
	}

	void before_closing () {
		save_preferences ();
	}

	void save_preferences () {
		var last_window = app.one_window ();
		if (get_realized ()) {
			if (!maximized && !fullscreened && get_mapped ()) {
				int width, height;
				get_default_size (out width, out height);
				app.settings.set_int (PREF_BROWSER_WINDOW_WIDTH, width);
				app.settings.set_int (PREF_BROWSER_WINDOW_HEIGHT, height);
			}

			app.settings.set_boolean (PREF_BROWSER_WINDOW_MAXIMIZED, maximized);
			app.settings.set_boolean (PREF_BROWSER_SIDEBAR_VISIBLE, browser_view.show_sidebar);
			app.settings.set_int (PREF_BROWSER_SIDEBAR_WIDTH, (int) browser_view.max_sidebar_width);
			app.settings.set_boolean (PREF_BROWSER_PROPERTIES_VISIBLE, browser_content_view.show_sidebar);
			app.settings.set_int (PREF_BROWSER_PROPERTIES_WIDTH, (int) browser_content_view.max_sidebar_width);

			if (last_window) {
				if (app.settings.get_boolean (PREF_BROWSER_GO_TO_LAST_LOCATION)
					&& (folder_tree.current_folder != null))
				{
					var uri = folder_tree.current_folder.file.get_uri ();
					app.settings.set_string (PREF_BROWSER_STARTUP_LOCATION, uri);
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
		}
	}

	unowned Gth.SidebarResizer active_resizer;

	[GtkChild] unowned Adw.OverlaySplitView browser_view;
	[GtkChild] unowned Adw.OverlaySplitView browser_content_view;
	[GtkChild] unowned Gth.FilterBar filter_bar;
	[GtkChild] unowned Gtk.MenuButton app_menu_button;
	[GtkChild] unowned Gtk.MenuButton bookmarks_button;
	[GtkChild] unowned Gtk.MenuButton history_button;
	[GtkChild] unowned Gtk.GridView file_grid;
	[GtkChild] public unowned Gth.FolderTree folder_tree;
	[GtkChild] public unowned Gth.Status status;
	[GtkChild] unowned Gtk.Stack folder_stack;
	[GtkChild] unowned Gtk.Widget non_empty_folder;
	[GtkChild] unowned Gtk.Widget empty_folder;
	[GtkChild] unowned Gtk.Widget show_sidebar_button;
	[GtkChild] unowned Gtk.ToggleButton vfs_button;
	[GtkChild] unowned Gtk.ToggleButton catalog_button;
	[GtkChild] unowned Adw.ToastOverlay browser_toast_overlay;
	[GtkChild] unowned Gtk.Stack sidebar_stack;
	[GtkChild] unowned Gtk.Stack second_sidebar_stack;
	[GtkChild] unowned Gth.PropertySidebar property_sidebar;
	[GtkChild] unowned Gtk.MenuButton location_button;
	[GtkChild] unowned Gth.SidebarResizer sidebar_resizer;
	[GtkChild] unowned Gth.SidebarResizer second_sidebar_resizer;
	[GtkChild] unowned Gtk.Button edit_catalog_button;
	[GtkChild] unowned Gtk.Button update_search_button;

	Page current_page = Page.NONE;
	public SimpleActionGroup action_group = null;
	Gth.Test general_filter;
	Gth.Test active_filter;
	HashTable<string, Gtk.Window?> named_dialogs;
	Gth.Thumbnailer thumbnailer;
	Gth.WindowHistory history;
	Gth.WindowBookmarks bookmarks;
	double initial_sidebar_width;
	double initial_second_sidebar_width;
}
