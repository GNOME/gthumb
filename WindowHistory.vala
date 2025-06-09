class Gth.WindowHistory {
	public weak Window window;
	public Menu menu;
	public GenericArray<File> files;
	public int current;

	public WindowHistory (Window _window) {
		window = _window;
		menu = null;
		files = new GenericArray<File>();
		current = -1;
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
			var item = Util.menu_item_for_file (file);
			item.set_action_and_target_value ("win.load-history-position", new Variant.int16 (idx));
			menu.append_item (item);
			if (current == idx) {
				var action = window.action_group.lookup_action ("load-history-position");
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
		if (window.current_folder != null) {
			add (window.current_folder.file);
		}
		update_menu ();
	}

	public void load (int idx) {
		if (!set_current (idx))
			return;
		var location = files[idx];
		if (location == null)
			return;
		window.open_location (location, LoadAction.OPEN_FROM_HISTORY);
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
		var action = window.action_group.lookup_action ("load-history-position");
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
		var action = window.action_group.lookup_action ("load-next") as SimpleAction;
		if (action != null) {
			action.set_enabled (can_load_next ());
		}
		action = window.action_group.lookup_action ("load-previous") as SimpleAction;
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
}
