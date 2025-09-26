public class Gth.FolderSelector : Object {
	public async File? select_folder (Gtk.Window? parent, File? current_folder = null, Cancellable? cancellable = null) throws Error {
		callback = select_folder.callback;
		dialog = new FolderSelectorDialog (current_folder);
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
		dialog.present (parent);
		yield;
		if (cancelled_event != 0) {
			cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		if (result == null) {
			throw new IOError.FAILED ("No folder selected.");
		}
		return result;
	}

	SourceFunc callback = null;
	FolderSelectorDialog dialog = null;
	ulong cancelled_event = 0;
	File? result = null;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/folder-selector-dialog.ui")]
class Gth.FolderSelectorDialog : Adw.Dialog {
	public signal void selected ();
	public File selected_folder;

	public FolderSelectorDialog (File? _selected_folder = null) {
		if (_selected_folder != null)
			selected_folder = _selected_folder;
		else
			selected_folder = Files.get_home ();
		folder_tree.job_queue = app.jobs;
		folder_tree.load.connect ((location, action) => {
			load_folder.begin (location, action);
		});
		folder_tree.load_folder.begin (selected_folder);
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
		selected_folder = folder_tree.get_selected ();
		if (selected_folder != null) {
			selected ();
		}
	}

	async void load_folder (File location, LoadAction load_action) throws Error {
		if (load_action == LoadAction.OPEN_AS_ROOT) {
			selected_folder = location;
			selected ();
			return;
		}
		yield folder_tree.load_folder (location, load_action);
	}

	[GtkChild] unowned Gth.FolderTree folder_tree;
}
