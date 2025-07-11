public class Gth.Bookmarks {
	public GenericList<BookmarkEntry> entries;
	public Menu menu;
	public Menu system_menu;
	public Menu roots_menu;

	bool loaded = false;

	public Bookmarks () {
		entries = new GenericList<BookmarkEntry> ();
		menu = new Menu ();
		system_menu = new Menu ();
		roots_menu = new Menu ();
	}

	public async void load_from_file () {
		if (loaded)
			return;
		yield load_app_bookmarks ();
		yield load_system_bookmarks ();
		yield update_root_menu ();
		loaded = true;
	}

	public async void load_app_bookmarks () {
		menu.remove_all ();
		var local_job = app.new_job ("Load bookmarks");
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
				var entry = new BookmarkEntry (file, name);
				entries.model.append (entry);
				var menu_item = Bookmarks.new_menu_item_from_entry (entry);
				menu.append_item (menu_item);
			}
		}
		catch (Error error) {
			// local_job.error = error;
		}
		local_job.done ();
	}

	public static MenuItem new_menu_item_from_entry (BookmarkEntry entry) {
		var menu_item = Util.menu_item_for_file (entry.file, entry.display_name);
		menu_item.set_action_and_target_value ("win.load-location", new Variant.string (entry.file.get_uri ()));
		return menu_item;
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
		var local_job = app.new_job ("Load system bookmarks");
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

	public async void save_app_bookmarks () throws Error {
		var local_job = app.new_job ("Save bookmarks");
		try {
			var bookmark_content = new BookmarkFile ();
			foreach (unowned var entry in entries) {
				var uri = entry.file.get_uri ();
				bookmark_content.set_is_private (uri, true);
				bookmark_content.add_application (uri, null, null);
				bookmark_content.set_title (uri, entry.display_name);
			}
			var content = bookmark_content.to_data (null);
			var bytes = new Bytes.static (content.data);
			var file = UserDir.get_config_file (FileIntent.WRITE, BOOKMARKS_FILE);
			yield Files.save_file_async (file, bytes, local_job.cancellable);
		}
		catch (Error error) {
			local_job.error = error;
		}
		finally {
			local_job.done ();
		}
		if (local_job.error != null) {
			throw local_job.error;
		}
	}
}

public class Gth.BookmarkEntry : Object {
	public File file;
	public string display_name { get; set; }

	public BookmarkEntry (File _file, string? _name) {
		file = _file;
		display_name = _name;
	}
}
