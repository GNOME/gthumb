public class Gth.FolderSelector : Object {
	public bool show_destination;

	public FolderSelector (FolderSelectorMode _mode = FolderSelectorMode.DEFAULT) {
		mode = _mode;
		show_destination = false;
	}

	public async File? select_folder (Gth.Window window, File? current_folder = null, Cancellable? cancellable = null) throws Error {
		callback = select_folder.callback;
		FileData root = (FolderSelectorMode.CATALOGS_ONLY in mode) ? Catalog.get_root () : null;
		dialog = new FolderSelectorDialog (window, root, mode, current_folder);
		dialog.set_show_destination (show_destination);
		dialog.selected.connect (() => {
			result = dialog.selected_folder;
			dialog.close ();
		});
		dialog.closed.connect (() => {
			if (callback != null) {
				dialog.release_resources ();
				Idle.add ((owned) callback);
				callback = null;
			}
		});
		if (cancellable != null) {
			cancelled_event = cancellable.cancelled.connect (() => {
				dialog.close ();
			});
		}
		dialog.present (window);
		yield;
		if (cancelled_event != 0) {
			cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		if (result == null) {
			throw new IOError.CANCELLED ("No folder selected.");
		}
		show_destination = dialog.show_destination;
		return result;
	}

	FolderSelectorMode mode;
	SourceFunc callback = null;
	FolderSelectorDialog dialog = null;
	ulong cancelled_event = 0;
	File? result = null;
}

[Flags]
public enum FolderSelectorMode {
	DEFAULT,
	FOR_MOVING,
	FOR_COPYING,
	CATALOGS_ONLY,
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/folder-selector-dialog.ui")]
class Gth.FolderSelectorDialog : Adw.Dialog {
	public signal void selected ();
	public File selected_folder;
	public bool show_destination;

	public FolderSelectorDialog (Gth.Window _window, FileData? root, FolderSelectorMode _mode, File? _selected_folder = null) {
		window = _window;
		mode = _mode;
		selected_folder = _selected_folder;
		show_destination = false;
		if (root != null) {
			folder_tree.set_root (root);
			if (selected_folder == null) {
				selected_folder = root.file;
			}
		}
		else if (_selected_folder == null) {
			selected_folder = Files.get_home ();
		}

		if (FolderSelectorMode.CATALOGS_ONLY in mode) {
			show_hidden_button.visible = false;
			new_catalog_button.visible = true;
			new_library_button.visible = true;
			if (FolderSelectorMode.FOR_COPYING in mode) {
				show_destination_switch.visible = true;
			}
		}

		if (FolderSelectorMode.FOR_COPYING in mode) {
			select_button.label = _("_Add");
		}
		else if (FolderSelectorMode.FOR_MOVING in mode) {
			select_button.label = _("_Move");
		}

		folder_tree.job_queue = app.jobs;
		folder_tree.skip_nochild_folders = (FolderSelectorMode.FOR_MOVING in mode);
		folder_tree.load.connect ((location, action) => {
			load_folder.begin (location, action);
		});
		folder_tree.load_folder.begin (selected_folder, LoadAction.OPEN, null, (_obj, res) => {
			try {
				folder_tree.load_folder.end (res);
			}
			catch (Error error) {
				if (error is IOError.NOT_FOUND) {
					selected_folder = (root != null) ? root.file : Files.get_home ();
					folder_tree.load_folder.begin (selected_folder);
				}
				else {
					show_message (error.message);
				}
			}
		});
	}

	public void set_show_destination (bool active) {
		show_destination_switch.active = active;
	}

	public void release_resources () {
		folder_tree.release_resources ();
	}

	[GtkCallback]
	void on_cancel (Gtk.Button button) {
		close ();
	}

	[GtkCallback]
	void on_select (Gtk.Button button) {
		try_select_folder (folder_tree.get_selected_file_data ());
	}

	void try_select_folder (FileData file_data) {
		try {
			if (file_data == null) {
				throw new IOError.FAILED (_("No file selected"));
			}
			if (FolderSelectorMode.FOR_COPYING in mode) {
				if (file_data.info.get_content_type () == "gthumb/library") {
					// Cannot copy to libraries.
					throw new IOError.FAILED (_("Invalid location"));
				}
				show_destination = show_destination_switch.active;
			}
			selected_folder = file_data.file;
			selected ();
		}
		catch (Error error) {
			show_message (error.message);
		}
	}

	async void load_folder (FileData location, LoadAction load_action) throws Error {
		if (load_action == LoadAction.OPEN_AS_ROOT) {
			try_select_folder (location);
			return;
		}
		yield folder_tree.load_folder (location.file, load_action);
	}

	[GtkCallback]
	void on_new_catalog (Adw.ButtonRow button) {
		new_catalog.begin ();
	}

	[GtkCallback]
	void on_new_library (Adw.ButtonRow button) {
		new_library.begin ();
	}

	async void new_catalog () {
		var local_job = window.new_job ("New Catalog");
		try {
			var dialog = new Gth.NewCatalogDialog ();
			var catalog = yield dialog.new_catalog (get_context_library (), window, local_job);
			yield folder_tree.load_folder (catalog, LoadAction.OPEN_NEW_FOLDER);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	async void new_library () {
		var local_job = window.new_job ("New Catalog");
		try {
			var dialog = new Gth.NewCatalogDialog ();
			var catalog = yield dialog.new_library (get_context_library (), window, local_job);
			yield folder_tree.load_folder (catalog, LoadAction.OPEN_NEW_FOLDER);
		}
		catch (Error error) {
			show_message (error.message);
		}
		finally {
			local_job.done ();
		}
	}

	File get_context_library () {
		File parent;
		if (folder_tree.current_folder.info.get_attribute_boolean ("gthumb::no-child")) {
			parent = folder_tree.current_folder.file.get_parent ();
		}
		else {
			parent = folder_tree.current_folder.file;
		}
		return parent;
	}

	void show_message (string message) {
		toast_overlay.dismiss_all ();
		toast_overlay.add_toast (Util.new_literal_toast (message));
	}

	[GtkChild] unowned Gth.FolderTree folder_tree;
	[GtkChild] unowned Adw.SwitchRow show_hidden_button;
	[GtkChild] unowned Adw.ButtonRow new_catalog_button;
	[GtkChild] unowned Adw.ButtonRow new_library_button;
	[GtkChild] unowned Adw.ToastOverlay toast_overlay;
	[GtkChild] unowned Adw.SwitchRow show_destination_switch;
	[GtkChild] unowned Gtk.Button select_button;
	Gth.Window window;
	FolderSelectorMode mode;
}
