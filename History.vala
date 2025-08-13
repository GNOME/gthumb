class Gth.History {
	public weak Browser browser;
	public Menu menu;
	public Gth.ActionList actions;
	public GenericArray<File> files;
	public int current;

	public History (Browser _browser) {
		browser = _browser;
		menu = null;
		actions = null;
		files = new GenericArray<File>();
		current = -1;
		locations_category = new ActionCategory (_("History"), 1);
		actions_category = new ActionCategory ("", 0);
	}

	public void print () {
		stdout.printf ("HISTORY:\n");
		var idx = 0;
		foreach (unowned var file in files) {
			stdout.printf ("  %s%s\n", (idx == current) ? "*" : " ", file.get_uri ());
			idx++;
		}
	}

	public void copy (History other) {
		files.length = 0;
		foreach (unowned var file in other.files) {
			files.add (file);
		}
		set_current (0);
		update_menu ();
	}

	public void update_menu () {
		actions.remove_all_actions ();

		var action = new ActionInfo ("win.delete-history", null, _("_Delete History"));
		action.category = actions_category;
		actions.append_action (action);

		var idx = 0;
		foreach (unowned var file in files) {
			var file_source = app.get_source_for_file (file);
			var info = file_source.get_display_info (file);
			var file_action = new ActionInfo ("win.load-history-position",
				new Variant.int16 (idx),
				info.get_display_name (),
				info.get_symbolic_icon ());
			file_action.category = locations_category;
			actions.append_action (file_action);
			if (current == idx) {
				var history_action = browser.window.action_group.lookup_action ("load-history-position");
				if (history_action != null) {
					history_action.change_state (new Variant.int16 (idx));
				}
			}
			idx++;
		}
	}

	public void add (File file) {
		if ((current != -1) && file.equal (files[current]))
			return;
		for (var idx = 0; idx < current; idx++) {
			files.remove_index (0);
		}
		while (files.length >= MAX_HISTORY_LENGTH) {
			files.remove_index (files.length - 1);
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
		if (browser.folder_tree.current_folder != null) {
			add (browser.folder_tree.current_folder.file);
		}
		update_menu ();
	}

	public void load (int idx) {
		if (!set_current (idx))
			return;
		var location = files[idx];
		if (location == null)
			return;
		browser.open_location (location, LoadAction.OPEN_FROM_HISTORY);
	}

	public void load_previous () {
		if (can_load_previous ()) {
			load (current + 1);
		}
	}

	public void load_next () {
		if (can_load_next ()) {
			load (current - 1);
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
				bookmarks.add_application (uri, "", "");
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

	public void restore_from_file () {
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
				//stdout.printf ("uri: %s\n", uri);
				if (Strings.empty (uri))
					continue;
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
		if ((idx < 0) || (idx >= files.length))
			return false;
		var action = browser.window.action_group.lookup_action ("load-history-position");
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

	void update_sensitivity () {
		var action = browser.window.action_group.lookup_action ("load-next") as SimpleAction;
		if (action != null) {
			action.set_enabled (can_load_next ());
		}
		action = browser.window.action_group.lookup_action ("load-previous") as SimpleAction;
		if (action != null) {
			action.set_enabled (can_load_previous ());
		}
	}

	bool can_load_previous () {
		return (files.length > 0) && (current < files.length - 1);
	}

	bool can_load_next () {
		return (files.length > 0) && (current > 0);
	}

	ActionCategory locations_category;
	ActionCategory actions_category;
}
