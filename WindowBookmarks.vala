class Gth.WindowBookmarks {
	public weak Window window;
	public Menu menu;
	public Menu system_menu;
	public Menu roots_menu;

	public WindowBookmarks (Window _window) {
		window = _window;
	}

	public async void load_from_file () {
		yield load_app_bookmarks ();
		yield load_system_bookmarks ();
		yield update_root_menu ();
	}

	public async void load_app_bookmarks () {
		menu.remove_all ();
		var local_job = window.new_job ("Load bookmarks");
		try {
			var bookmarks_file = UserDir.get_config_file (FileIntent.READ, BOOKMARKS_FILE);
			var bytes = yield Files.load_file_async (bookmarks_file, local_job.cancellable);
			var bookmarks = new BookmarkFile ();
			bookmarks.load_from_data ((string) bytes.get_data (), bytes.length);
			var uris = bookmarks.get_uris ();
			foreach (unowned var uri in uris) {
				if (Strings.empty (uri))
					continue;
				var file = File.new_for_uri (uri);
				var name = bookmarks.get_title (uri);
				var menu_item = Util.menu_item_for_file (file, name);
				menu_item.set_action_and_target_value ("win.load-location", new Variant.string (uri));
				menu.append_item (menu_item);
			}
		}
		catch (Error error) {
			// local_job.error = error;
		}
		local_job.done ();
	}

	public async void load_system_bookmarks () {
		system_menu.remove_all ();
#if FLATPAK_BUILD
		var path = Path.build_filename (g_get_home (), ".config", "gtk-3.0", "bookmarks");
		var bookmarks_file = File.new_for_path (path);
#else
		var bookmarks_dir = UserDir.get_directory (FileIntent.READ, DirType.CONFIG, "gtk-3.0");
		var bookmarks_file = bookmarks_dir.get_child ("bookmarks");
#endif
		var local_job = window.new_job ("Load system bookmarks");
		try {
			var contents = yield Files.load_contents_async (bookmarks_file, local_job.cancellable);
			var lines = contents.split ("\n");
			foreach (unowned var line in lines) {
				var components = line.split (" ", 2);
				unowned var uri = components[0];
				if (Strings.empty (uri))
					continue;
				var after_space = line.index_of_char (' ');
				unowned var name = (after_space > 0) ? Strings.unowned_substring (line, after_space + 1) : null;
				var menu_item = Util.menu_item_for_file (File.new_for_uri (uri), name);
				menu_item.set_action_and_target_value ("win.load-location", new Variant.string (uri));
				system_menu.append_item (menu_item);
			}
		}
		catch (Error error) {
			// local_job.error = error;
		}
		local_job.done ();
	}

	public async void update_root_menu () {
		var roots = yield app.update_roots ();
		roots_menu.remove_all ();
		foreach (unowned var file_data in roots) {
			var uri = file_data.file.get_uri ();
			var menu_item = new MenuItem (null, null);
			menu_item.set_label (file_data.info.get_display_name ());
			menu_item.set_icon (file_data.info.get_symbolic_icon ());
			menu_item.set_action_and_target_value ("win.load-location", new Variant.string (uri));
			roots_menu.append_item (menu_item);
		}
	}
}
