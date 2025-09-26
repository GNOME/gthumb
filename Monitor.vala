public class Gth.Monitor : Object {
	public Monitor () {
		file_monitors = new HashTable<File, FileMonitor> (Files.hash, Files.equal);
		created_files = new GenericArray<File>();
		deleted_files = new GenericArray<File>();
		changed_files = new GenericArray<File>();
	}

	public signal void bookmarks_changed () {
		app.foreach_window ((win) => win.browser.update_bookmarks_menu ());
	}

	public signal void catalog_saved (Gth.Catalog catalog, File old_file) {
		app.foreach_window ((win) => win.browser.catalog_saved (catalog, old_file));
	}

	public void metadata_changed (Gth.FileData file_data) {
		app.foreach_window ((win) => win.metadata_changed (file_data));
	}

	public signal void file_changed (File file);

	public signal void files_deleted (GenericList<File> files) {
		app.foreach_window ((win) => {
			win.browser.files_deleted (files);
			win.viewer.files_deleted (files);
		});
	}

	public signal void files_created (File parent, GenericList<File> files) {
		app.foreach_window ((win) => win.browser.files_created (parent, files));
	}

	public void file_created (File file) {
		var parent = file.get_parent ();
		if (parent == null) {
			return;
		}
		var files = new GenericList<File>();
		files.model.append (file);
		files_created (parent, files);
	}

	public void file_deleted (File file) {
		var files = new GenericList<File>();
		files.model.append (file);
		files_deleted (files);
	}

	public void scripts_changed () {
		app.foreach_window ((win) => win.update_scripts_actions ());
	}

	public void watch_file (File file, bool watch) {
		if (watch) {
			stdout.printf ("> START WATCH FILE: %s\n", file.get_uri ());
			if (!file_monitors.contains (file)) {
				var file_monitor = file.monitor_directory (FileMonitorFlags.NONE, null);
				if (file_monitor != null) {
					file_monitor.changed.connect (on_file_changed);
					file_monitors.set (file, file_monitor);
				}
			}
		}
		else {
			stdout.printf ("> STOP WATCH FILE: %s\n", file.get_uri ());
			file_monitors.remove (file);
		}
	}

	void on_file_changed (File file, File? other_file, FileMonitorEvent event_type) {
		switch (event_type) {
		case FileMonitorEvent.DELETED, FileMonitorEvent.UNMOUNTED:
			event_type = FileMonitorEvent.DELETED;
			break;

		case FileMonitorEvent.CHANGED,
			FileMonitorEvent.CHANGES_DONE_HINT,
			FileMonitorEvent.ATTRIBUTE_CHANGED:
			event_type = FileMonitorEvent.CHANGED;
			break;

		default:
			event_type = FileMonitorEvent.CREATED;
			break;
		}

		if (event_type == FileMonitorEvent.CREATED) {
			if (deleted_files.remove (file)) {
				event_type = FileMonitorEvent.CHANGED;
			}
		}
		else if (event_type == FileMonitorEvent.DELETED) {
			created_files.remove (file);
			changed_files.remove (file);
		}
		else if (event_type == FileMonitorEvent.CHANGED) {
			if (created_files.find_with_equal_func (file, Files.equal))
				return;
			changed_files.remove (file);
		}

		switch (event_type) {
		case FileMonitorEvent.CREATED:
			created_files.add (file);
			break;

		case FileMonitorEvent.DELETED:
			deleted_files.add (file);
			break;

		case FileMonitorEvent.CHANGED:
			changed_files.add (file);
			break;
		}

		if (update_id != 0) {
			Source.remove (update_id);
		}
		update_id = Util.after_timeout (500, () => {
			update_id = 0;
			process_events ();
		});
	}

	void process_events () {
		var local_changed = new GenericArray<File>();
		foreach (unowned var file in changed_files) {
			local_changed.add (file);
		}
		changed_files.length = 0;
		foreach (unowned var file in local_changed) {
			file_changed (file);
		}

		var local_deleted = new GenericList<File>();
		foreach (unowned var file in deleted_files) {
			local_deleted.model.append (file);
		}
		deleted_files.length = 0;
		files_deleted (local_deleted);

		var local_created = new GenericArray<File>();
		foreach (unowned var file in created_files) {
			local_created.add (file);
		}
		created_files.length = 0;
		File last_parent = null;
		var created_with_same_parent = new GenericList<File>();
		foreach (unowned var file in local_created) {
			var parent = file.get_parent ();
			if (last_parent == null) {
				last_parent = parent;
			}
			if (parent == last_parent) {
				created_with_same_parent.model.append (file);
			}
			else {
				if (created_with_same_parent.length () > 0) {
					files_created (last_parent, created_with_same_parent);
					created_with_same_parent = new GenericList<File>();
				}
				last_parent = null;
			}
		}
		if (created_with_same_parent.length () > 0) {
			files_created (last_parent, created_with_same_parent);
		}
	}

	public void release_resources () {
		if (update_id != 0) {
			Source.remove (update_id);
			update_id = 0;
		}
		file_monitors.remove_all ();
	}

	HashTable<File, FileMonitor> file_monitors;
	GenericArray<File> created_files;
	GenericArray<File> deleted_files;
	GenericArray<File> changed_files;
	uint update_id = 0;
}
