public class Gth.RenameFiles : Object {
	public async void rename (Gtk.Window? parent, GenericList<File> files, Job job) throws Error {
		callback = rename.callback;
		dialog = new RenameDialog (files);
		dialog.saved.connect (() => {
			dialog.close ();
			saved = true;
		});
		dialog.closed.connect (() => {
			if (callback != null) {
				Idle.add ((owned) callback);
				callback = null;
			}
		});
		cancelled_event = job.cancellable.cancelled.connect (() => {
			dialog.close ();
		});
		job.opens_dialog ();
		dialog.present (parent);
		yield;
		job.dialog_closed ();
		if (cancelled_event != 0) {
			job.cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		if (!saved) {
			throw new IOError.CANCELLED ("Cancelled");
		}
	}

	SourceFunc callback = null;
	RenameDialog dialog = null;
	ulong cancelled_event = 0;
	bool saved = false;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/rename-dialog.ui")]
public class Gth.RenameDialog : Adw.PreferencesDialog {
	public signal void saved ();

	public RenameDialog (GenericList<File> _files) {
		files = _files;
	}

	// [GtkCallback]
	// void on_cancel (Gtk.Button button) {
	// 	close ();
	// }

	// [GtkCallback]
	// void on_save (Gtk.Button button) {
	// 	update_file_attributes ();
	// 	saved ();
	// }

	GenericList<File> files;
}
