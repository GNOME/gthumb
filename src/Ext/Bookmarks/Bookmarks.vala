public class Gth.Bookmarks {
	public GenericList<ActionInfo> roots;
	public GenericList<ActionInfo> app_bookmarks;
	public GenericList<ActionInfo> system_bookmarks;

	public Bookmarks () {
		roots = new GenericList<ActionInfo> ();
		app_bookmarks = new GenericList<ActionInfo> ();
		system_bookmarks = new GenericList<ActionInfo> ();
		roots_category = new ActionCategory (_("Locations"), 0);
		bookmarks_category = new ActionCategory (_("Bookmarks"), 1);
		system_bookmarks_category = new ActionCategory (_("System Bookmarks"), 2);
		locations = new GenericSet<File> (Util.file_hash, Util.file_equal);
		loaded = false;
		app.events.mount_points_changed.connect (() => on_mount_points_changed.begin ());
	}

	public async void load_from_file () {
		if (loaded) {
			return;
		}
		locations.remove_all ();
		yield update_root_menu ();
		yield load_app_bookmarks ();
		yield load_system_bookmarks ();
		loaded = true;
		app.events.bookmarks_changed ();
	}

	async void on_mount_points_changed () {
		if (!loaded) {
			return;
		}
		yield update_root_menu ();
		app.events.bookmarks_changed ();
	}

	async void load_app_bookmarks () {
		app_bookmarks.model.remove_all ();
		var local_job = app.jobs.new_job ("Loading Bookmarks");
		try {
			var file = UserDir.get_config_file (FileIntent.READ, BOOKMARKS_FILE);
			size_t contents_size;
			var contents = yield Files.load_contents_async (file, local_job.cancellable, out contents_size);
			var bookmark_file = new BookmarkFile ();
			bookmark_file.load_from_data (contents, contents_size);
			var uris = bookmark_file.get_uris ();
			foreach (unowned var uri in uris) {
				if (Strings.empty (uri))
					continue;
				var entry_file = File.new_for_uri (uri);
				var name = bookmark_file.get_title (uri);
				var entry = new ActionInfo.for_file ("win.load-location", entry_file, name);
				entry.category = bookmarks_category;
				app_bookmarks.model.append (entry);
				locations.add (entry_file);
			}
		}
		catch (Error error) {
			// local_job.error = error;
		}
		local_job.done ();
	}

	async void load_system_bookmarks () {
		system_bookmarks.model.remove_all ();
#if FLATPAK_BUILD
		var path = Path.build_filename (g_get_home (), ".config", "gtk-3.0", "bookmarks");
		var bookmarks_file = File.new_for_path (path);
#else
		var bookmarks_dir = UserDir.get_directory (FileIntent.READ, DirType.CONFIG, "gtk-3.0");
		var bookmarks_file = bookmarks_dir.get_child ("bookmarks");
#endif
		var local_job = app.jobs.new_job ("Loading System Bookmarks");
		try {
			var contents = yield Files.load_contents_async (bookmarks_file, local_job.cancellable);
			var lines = contents.split ("\n");
			foreach (unowned var line in lines) {
				var components = line.split (" ", 2);
				unowned var uri = components[0];
				if (Strings.empty (uri)) {
					continue;
				}
				var file = File.new_for_uri (uri);
				if (locations.contains (file)) {
					continue;
				}
				var after_space = line.index_of_char (' ');
				unowned var name = (after_space > 0) ? Strings.unowned_substring (line, after_space + 1) : null;
				var action = new ActionInfo.for_file ("win.load-location", file, name);
				action.category = system_bookmarks_category;
				system_bookmarks.model.append (action);
				locations.add (file);
			}
		}
		catch (Error error) {
			// local_job.error = error;
		}
		local_job.done ();
	}

	async void update_root_menu () {
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
			locations.add (file_data.file);
		}
	}

	public async void save_app_bookmarks () throws Error {
		var local_job = app.jobs.new_job ("Saving Bookmarks");
		try {
			var bookmark_content = new BookmarkFile ();
			foreach (unowned var entry in app_bookmarks) {
				var uri = entry.value.get_string ();
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
			app.events.bookmarks_changed ();
		}
		if (local_job.error != null) {
			throw local_job.error;
		}
	}

	public async void add_bookmark (File file) throws Error {
		var file_uri = file.get_uri ();
		foreach (unowned var entry in app_bookmarks) {
			var uri = entry.value.get_string ();
			if (uri == file_uri) {
				throw new IOError.FAILED (_("Location already saved"));
			}
		}
		var entry = new ActionInfo.for_file ("win.load-location", file);
		entry.category = bookmarks_category;
		app_bookmarks.model.append (entry);
		yield save_app_bookmarks ();
	}

	ActionCategory roots_category;
	ActionCategory bookmarks_category;
	ActionCategory system_bookmarks_category;
	GenericSet<File> locations;
	bool loaded;
}
