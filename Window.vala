[GtkTemplate (ui = "/app/gthumb/gthumb/ui/window.ui")]
public class Gth.Window : Adw.ApplicationWindow {
	public FileData folder = null;
	public GenericList<FileData> all_children = null;
	public GenericList<FileData> visible_files = null;
	public FileSource folder_source = null;
	public string sort_name = null;
	public bool inverse_order = false;
	public bool fast_file_type = false;
	public bool show_hidden_files = false;
	public bool sidebar_visible = false;
	public bool sidebar_pinned = false;
	public int thumbnail_size;
	public Gth.JobQueue jobs;

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
		thumbnailer = new Thumbnailer ();
		history = new FileHistory (this);

		filter_bar.changed.connect (() => update_active_filter ());

		file_grid.model = new Gtk.MultiSelection (visible_files.model);
		file_grid.model.selection_changed.connect (() => {
			update_selection_info ();
		});

		browser_view.notify["show-sidebar"].connect (() => {
			sidebar_visible = browser_view.show_sidebar;
			action_group.change_action_state ("show-sidebar", new Variant.boolean (sidebar_visible));
			show_sidebar_button.visible = !sidebar_visible;
		});

		var file_item_factory = new Gtk.SignalListItemFactory ();
		file_item_factory.setup.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			list_item.child = new Gth.FileListItem (thumbnail_size, thumbnail_attributes_v);
		});
		file_item_factory.teardown.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			list_item.child = null;
		});
		file_item_factory.bind.connect ((obj) => {
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
		file_item_factory.unbind.connect ((obj) => {
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
		file_grid.factory = file_item_factory;

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

		init_actions ();
		set_page (Page.BROWSER);

		// Load the location.
		title = "Thumbnails";
		Util.next_tick (() => {
			if (app.one_window ())
				history.load_from_file ();
			filter_bar.set_active_filter (active_filter);
			load_location (location, LoadAction.LOAD, file_to_select);
		});
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
		var local_job = new_job ("Load %s".printf (location.get_uri ()));
		load_job = local_job;
		try {
			var file_data = yield source.read_metadata (location, "*", local_job.cancellable);
			var children = yield source.list_children (location, get_list_attributes (true), local_job.cancellable);

			folder = file_data;
			folder_source = source;
			all_children = children;
			// TODO source.monitor_directory (folder.file, true);
			update_thumbnail_list ();
			update_title ();
			if (load_action != LoadAction.LOAD_FROM_HISTORY) {
				history.add (folder.file);
			}
		}
		catch (Error error) {
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

	public void load_location (File location, LoadAction load_action = LoadAction.LOAD, File? file_to_select = null) {
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
		if (folder != null) {
			title = folder.info.get_display_name ();
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

	void update_thumbnail_list () {
		// Filter the files.
		var visible_children = new GenericList<FileData>();
		var filter = get_file_filter (true);
		var iter = filter.iterator (all_children);
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

		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/history-menu.ui");
		history_button.menu_model = builder.get_object ("history_menu") as MenuModel;
		history.menu = builder.get_object ("history_entries") as Menu;

		action_group = new SimpleActionGroup ();
		insert_action_group ("win", action_group);

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

		action = new SimpleAction.stateful ("set-filter", VariantType.STRING, new Variant.string ((active_filter != null) ? active_filter.id : ""));
		action.activate.connect ((_action, param) => {
			_action.set_state (param);
			filter_bar.select_filter_by_id (param.get_string ());
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
			var window = new Gth.Window (app, folder.file, null);
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
			sidebar_pinned = Util.toggle_state (_action);
			browser_view.collapsed = !sidebar_pinned;
			if (browser_view.collapsed) {
				browser_view.show_sidebar = true;
			}
			show_sidebar_button.visible = !sidebar_visible;
		});
		action_group.add_action (action);

		action = new SimpleAction ("go-home", null);
		action.activate.connect ((_action, param) => {
			load_location (Files.get_home_dir ()); // TODO: use the prefereces location
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("go-to-history-position", VariantType.INT16, new Variant.int16 ((int16) history.current));
		action.activate.connect ((_action, param) => {
			history.go_to (param.get_int16 ());
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
	}

	[GtkChild] unowned Adw.OverlaySplitView browser_view;
	[GtkChild] unowned Gth.FilterBar filter_bar;
	[GtkChild] unowned Gtk.MenuButton app_menu_button;
	[GtkChild] unowned Gtk.MenuButton bookmarks_button;
	[GtkChild] unowned Gtk.MenuButton history_button;
	[GtkChild] unowned Gtk.GridView file_grid;
	[GtkChild] public unowned Gth.Status status;
	[GtkChild] unowned Gtk.Stack folder_stack;
	[GtkChild] unowned Gtk.Widget non_empty_folder;
	[GtkChild] unowned Gtk.Widget empty_folder;
	[GtkChild] unowned Gtk.Widget show_sidebar_button;

	Page current_page = Page.NONE;
	public SimpleActionGroup action_group;
	Gth.Test general_filter;
	Gth.Test active_filter;
	HashTable<string, Gtk.Window?> named_dialogs;
	Gth.Thumbnailer thumbnailer;
	Gth.FileHistory history;
}

class Gth.FileHistory {
	public GenericArray<File> files;
	public int current;
	public Menu menu;
	public weak Window window;

	public FileHistory (Window _window) {
		window = _window;
		files = new GenericArray<File>();
		current = -1;
		menu = null;
	}

	public bool can_go_backward () {
		return (files.length > 0) && (current > 0);
	}

	public bool can_go_forward () {
		return (files.length > 0) && (current < files.length - 1);
	}

	public void print () {
		stdout.printf ("HISTORY:\n");
		var idx = 0;
		foreach (unowned var file in files) {
			stdout.printf ("  %s%s\n", (idx == current) ? "*" : " ", file.get_uri ());
			idx++;
		}
	}

	public void update_menu () {
		menu.remove_all ();
		var idx = 0;
		foreach (unowned var file in files) {
			var item = menu_item_for_file (file);
			item.set_action_and_target_value ("win.go-to-history-position", new Variant.int16 (idx));
			menu.append_item (item);
			if (current == idx) {
				var action = window.action_group.lookup_action ("go-to-history-position");
				if (action != null) {
					action.change_state (new Variant.int16 (idx));
				}
			}
			idx++;
		}
		update_sensitivity ();
	}

	public void add (File file) {
		if ((current != -1) && file.equal (files[current]))
			return;
		for (var idx = 0; idx < current; idx++) {
			files.remove_index (0);
		}
		files.insert (0, file);
		current = 0;
		update_menu ();
	}

	public File? get (int index) {
		return files[index];
	}

	public void clear () {
		files.length = 0;
		current = -1;
		if (window.folder != null) {
			add (window.folder.file);
		}
		update_menu ();
	}

	public void go_to (int idx) {
		if (!set_current (idx))
			return;
		var location = files[idx];
		if (location == null)
			return;
		window.load_location (location, LoadAction.LOAD_FROM_HISTORY);
	}

	public void load_previous () {
		if (can_go_backward ()) {
			go_to (current - 1);
		}
	}

	public void load_next () {
		if (can_go_forward ()) {
			go_to (current + 1);
		}
	}

	const int MAX_HISTORY_LENGTH = 15;

	public void save_to_file () {
		var settings = Util.get_settings_if_schema_installed ("org.gnome.desktop.privacy");
		if (settings == null)
			return;

		var save_history = settings.get_boolean ("remember-recent-files");
		if (!save_history) {
			try {
				var bookmarks_file = get_history_file (FileIntent.READ);
				bookmarks_file.delete ();
			}
			catch (Error error) {
				// Ignored.
			}
		}

		var bookmarks_file = get_history_file (FileIntent.WRITE);
		if (bookmarks_file == null)
			return;

		try {
			var bookmarks = new BookmarkFile ();
			var idx = 0;
			foreach (unowned var file in files) {
				var uri = file.get_uri ();
				bookmarks.set_is_private (uri, true);
				bookmarks.add_application (uri, null, null);
				idx++;
				if (idx >= MAX_HISTORY_LENGTH) {
					break;
				}
			}
			bookmarks.to_file (bookmarks_file.get_path ());
		}
		catch (Error error) {
			// Ignored.
		}
	}

	public void load_from_file () {
		var settings = Util.get_settings_if_schema_installed ("org.gnome.desktop.privacy");
		if (settings == null)
			return;

		var load_history = settings.get_boolean ("remember-recent-files");
		if (!load_history)
			return;

		try {
			var bookmarks = new BookmarkFile ();
			var bookmarks_file = get_history_file (FileIntent.READ);
			bookmarks.load_from_file (bookmarks_file.get_path ());
			var idx = 0;
			var uris = bookmarks.get_uris ();
			foreach (unowned var uri in uris) {
				stdout.printf ("uri: %s\n", uri);
				files.add (File.new_for_uri (uri));
				idx++;
				if (idx >= MAX_HISTORY_LENGTH) {
					break;
				}
			}
			set_current (0);
			update_menu ();
		}
		catch (Error error) {
			// Ignored.
		}
	}

	bool set_current (int idx) {
		var action = window.action_group.lookup_action ("go-to-history-position");
		if (action == null)
			return false;
		action.change_state (new Variant.int16 ((int16) idx));
		current = idx;
		update_sensitivity ();
		return true;
	}

	File? get_history_file (FileIntent intent) {
		var dir = UserDir.get_directory (intent, DirType.CONFIG, APP_DIR);
		return (dir != null) ? dir.get_child (HISTORY_FILE) : null;
	}

	MenuItem menu_item_for_file (File file) {
		var file_source = app.get_source_for_file (file);
		var info = file_source.get_display_info (file);
		var item = new MenuItem (null, null);
		item.set_label (info.get_display_name ());
		item.set_icon (info.get_symbolic_icon ());
		return item;
	}

	void update_sensitivity () {
		var action = window.action_group.lookup_action ("load-next") as SimpleAction;
		if (action != null) {
			action.set_enabled (can_go_forward ());
		}
		action = window.action_group.lookup_action ("load-previous") as SimpleAction;
		if (action != null) {
			action.set_enabled (can_go_backward ());
		}
	}
}
