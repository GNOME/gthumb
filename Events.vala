public class Gth.Events : Object {
	public Events () {
		file_monitors = new HashTable<File, FileMonitor> (Util.file_hash, Util.file_equal);
		file_events = new FileEvents ();
		volume_monitor = VolumeMonitor.get ();
		volume_monitor.mount_changed.connect (on_mount_points_changed);
		volume_monitor.mount_added.connect (on_mount_points_changed);
		volume_monitor.mount_removed.connect (on_mount_points_changed);
		volume_monitor.volume_changed.connect (on_mount_points_changed);
		volume_monitor.volume_added.connect (on_mount_points_changed);
		volume_monitor.volume_removed.connect (on_mount_points_changed);
	}

	public signal void bookmarks_changed () {
		app.foreach_main_window ((win) => win.browser.update_bookmarks_menu ());
	}

	public signal void catalog_saved (Gth.Catalog catalog, File old_file) {
		app.foreach_main_window ((win) => win.browser.catalog_saved (catalog, old_file));
	}

	public void metadata_changed (File file) {
		app.foreach_main_window ((win) => win.browser.metadata_changed (file));
	}

	public signal void file_changed (File file) {
		app.foreach_main_window ((win) => win.browser.file_changed (file));
	}

	public signal void files_added (File parent, GenericList<File> files) {
		app.foreach_main_window ((win) => win.browser.files_added (parent, files));
	}

	public signal void files_added_or_changed (File parent, GenericList<File> files) {
		app.foreach_main_window ((win) => win.browser.files_added (parent, files, true));
	}

	public signal void files_removed_from_catalog (File parent, GenericList<File> files) {
		app.foreach_main_window ((win) => win.browser.files_removed_from_catalog (parent, files));
	}

	public signal void files_deleted_from_disk (GenericList<File> files) {
		app.foreach_main_window ((win) => {
			win.browser.files_deleted_from_disk (files);
			win.viewer.files_deleted_from_disk (files);
		});
	}

	public signal void files_renamed (GenericList<RenamedFile> files) {
		app.foreach_main_window ((win) => {
			win.browser.files_renamed (files);
		});
	}

	public signal void mount_points_changed ();

	public void file_created (File file) {
		var parent = file.get_parent ();
		if (parent == null) {
			return;
		}
		var files = new GenericList<File>();
		files.model.append (file);
		files_added (parent, files);
	}

	public void file_saved (File file) {
		var parent = file.get_parent ();
		if (parent == null) {
			return;
		}
		var files = new GenericList<File>();
		files.model.append (file);
		files_added_or_changed (parent, files);
	}

	public void file_deleted_from_disk (File file) {
		var files = new GenericList<File>();
		files.model.append (file);
		files_deleted_from_disk (files);
	}

	public void scripts_changed () {
		app.foreach_main_window ((win) => win.update_scripts_actions ());
	}

	public void selection_changed (uint number) {
		app.foreach_main_window ((win) => win.update_selection_status (number));
	}

	public void watch_file (File file, bool watch) {
		if (watch) {
			stdout.printf ("> START WATCH FILE: %s\n", file.get_uri ());
			if (!file_monitors.contains (file)) {
				try {
					var file_monitor = file.monitor_directory (FileMonitorFlags.NONE, null);
					file_monitor.changed.connect (on_file_changed);
					file_monitors.set (file, file_monitor);
				}
				catch (Error error) {
				}
			}
		}
		else {
			stdout.printf ("> STOP WATCH FILE: %s\n", file.get_uri ());
			file_monitors.remove (file);
		}
	}

	public void release_resources () {
		if (update_id != 0) {
			Source.remove (update_id);
			update_id = 0;
		}
		file_monitors.remove_all ();
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

		stdout.printf ("> MONITOR: %s: %s\n", event_type.to_string (), file.get_uri ());

		var event = file_events.get (file);
		if (event != null) {
			if (event_type == FileMonitorEvent.CREATED) {
				event.type = FileMonitorEvent.CREATED;
			}
			else if (event_type == FileMonitorEvent.DELETED) {
				event.type = FileMonitorEvent.DELETED;
			}
			else if (event_type == FileMonitorEvent.CHANGED) {
				if (event_type == FileMonitorEvent.CREATED) {
					// Stays CREATED
				}
				else if (event_type == FileMonitorEvent.DELETED) {
					event.type = FileMonitorEvent.CREATED;
				}
			}
			event.update_time ();
		}
		else {
			event = new FileEvent (file, event_type);
			file_events.events.add (event);
		}

		queue_process_file_events ();
	}

	void queue_process_file_events () {
		if (update_id != 0) {
			return;
		}
		update_id = Util.after_timeout (PROCESS_DELAY_MILLISECONDS, () => {
			update_id = 0;
			process_file_events ();
		});
	}

	const uint PROCESS_DELAY_MILLISECONDS = 2000;
	const uint PROCESS_DELAY_MICROSECONDS = PROCESS_DELAY_MILLISECONDS * 1000;

	void process_file_events () {
		var young_events = 0;
		var local_created = new GenericArray<File?>();
		var local_deleted = new GenericList<File>();
		var now = new GLib.DateTime.now_local ();
		var idx = 0;
		while (idx < file_events.events.length) {
			var event = file_events.events[idx];
			if (now.difference (event.time) < PROCESS_DELAY_MICROSECONDS) {
				idx++;
				young_events++;
				continue;
			}
			if (event.type == FileMonitorEvent.CREATED) {
				local_created.add (event.file);
			}
			else if (event.type == FileMonitorEvent.DELETED) {
				local_deleted.model.append (event.file);
			}
			else if (event.type == FileMonitorEvent.CHANGED) {
				file_changed (event.file);
			}
			file_events.events.remove_index (idx);
		}

		files_deleted_from_disk (local_deleted);

		// Group created files by parent.
		for (var i = 0; i < local_created.length; i++) {
			var file = local_created[i];
			if (file == null) {
				continue;
			}
			var parent = file.get_parent ();
			var same_parent = new GenericList<File>();
			same_parent.model.append (file);
			local_created[i] = null;
			for (var j = 0; j < local_created.length; j++) {
				var other_file = local_created[j];
				if (other_file != null) {
					var other_parent = other_file.get_parent ();
					if (other_parent.equal (parent)) {
						same_parent.model.append (other_file);
						local_created[j] = null;
					}
				}
			}
			files_added (parent, same_parent);
		}

		if (young_events > 0) {
			queue_process_file_events ();
		}
	}

	void on_mount_points_changed () {
		Util.next_tick (() => mount_points_changed ());
	}

	HashTable<File, FileMonitor> file_monitors;
	VolumeMonitor volume_monitor;
	FileEvents file_events;
	uint update_id = 0;
}


class Gth.FileEvents {
	public GenericArray<FileEvent> events;

	public FileEvents () {
		events = new GenericArray<FileEvent>();
	}

	public FileEvent? get (File file) {
		foreach (unowned var event in events) {
			if (event.file.equal (file)) {
				return event;
			}
		}
		return null;
	}
}


class Gth.FileEvent {
	public File file;
	public GLib.DateTime time;
	public GLib.FileMonitorEvent type;

	public FileEvent (File _file, GLib.FileMonitorEvent _type) {
		file = _file;
		type = _type;
		time = new GLib.DateTime.now_local ();
	}

	public void update_time () {
		time = new GLib.DateTime.now_local ();
	}
}
