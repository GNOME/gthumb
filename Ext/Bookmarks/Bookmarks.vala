public class Gth.Bookmarks {
	public GenericList<BookmarkEntry> entries;
	public GenericList<ActionInfo> roots;
	public GenericList<ActionInfo> system_bookmarks;

	public Bookmarks () {
		entries = new GenericList<BookmarkEntry> ();
		roots = new GenericList<ActionInfo> ();
		system_bookmarks = new GenericList<ActionInfo> ();
		roots_category = new ActionCategory (_("Locations"), 0);
		system_bookmarks_category = new ActionCategory (_("System Bookmarks"), 2);
		loaded = false;
	}

	public async void load_from_file () {
		if (loaded)
			return;
		yield load_app_bookmarks ();
		yield load_system_bookmarks ();
		yield update_root_menu ();
		loaded = true;
		app.monitor.bookmarks_changed ();
	}

	public async void load_app_bookmarks () {
		entries.model.remove_all ();
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
				entries.model.append (new BookmarkEntry (file, name));
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
		system_bookmarks.model.remove_all ();
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
				var action = new ActionInfo.for_file ("win.load-location", File.new_for_uri (uri), name);
				action.category = system_bookmarks_category;
				system_bookmarks.model.append (action);
			}
		}
		catch (Error error) {
			// local_job.error = error;
		}
		local_job.done ();
	}

	public async void update_root_menu () {
		var root_list = yield app.update_roots ();
		roots.model.remove_all ();
		foreach (unowned var file_data in root_list) {
			var action = new ActionInfo ("win.load-location",
				new Variant.string (file_data.file.get_uri ()),
				file_data.info.get_display_name (),
				file_data.info.get_symbolic_icon ()
			);
			action.category = roots_category;
			roots.model.append (action);
		}
	}

	public async void save_app_bookmarks () throws Error {
		var local_job = app.new_job ("Save bookmarks");
		try {
			var bookmark_content = new BookmarkFile ();
			foreach (unowned var entry in entries) {
				var uri = entry.file.get_uri ();
				bookmark_content.set_is_private (uri, true);
				bookmark_content.add_application (uri, "", "");
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
			app.monitor.bookmarks_changed ();
		}
		if (local_job.error != null) {
			throw local_job.error;
		}
	}

	public async void add_bookmark (File file) throws Error {
		foreach (unowned var entry in entries) {
			if (entry.file.equal (file)) {
				throw new IOError.FAILED (_("Location already saved"));
			}
		}
		var file_source = app.get_source_for_file (file);
		var info = file_source.get_display_info (file);
		var entry = new BookmarkEntry (file, info.get_display_name ());
		entries.model.append (entry);
		yield save_app_bookmarks ();
	}

	ActionCategory roots_category;
	ActionCategory system_bookmarks_category;
	bool loaded;
}

public class Gth.BookmarkEntry : Object {
	public File file;
	public string display_name { get; set; }

	public BookmarkEntry (File _file, string? _name) {
		file = _file;
		display_name = _name;
	}
}
