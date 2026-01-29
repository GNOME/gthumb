public class Gth.ConvertFormat : Object {
	public async FileOperation? convert (Gth.Window window, Job job) throws Error {
		callback = convert.callback;
		dialog = new ConvertDialog (window);
		dialog.saved.connect (() => {
			saved = true;
			dialog.close ();
		});
		dialog.closed.connect (() => {
			if (callback != null) {
				dialog.save_preferences ();
				Idle.add ((owned) callback);
				callback = null;
			}
		});
		cancelled_event = job.cancellable.cancelled.connect (() => {
			dialog.close ();
		});
		job.opens_dialog ();
		dialog.present (window);
		yield;
		job.dialog_closed ();
		if (cancelled_event != 0) {
			job.cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		if (!saved) {
			throw new IOError.CANCELLED ("Cancelled");
		}
		return dialog.get_operation ();
	}

	SourceFunc callback = null;
	ConvertDialog dialog = null;
	ulong cancelled_event = 0;
	bool saved = false;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/convert-dialog.ui")]
public class Gth.ConvertDialog : Adw.Dialog {
	public signal void saved ();

	public ConvertDialog (Gth.Window window) throws Error {
		settings = new GLib.Settings (GTHUMB_CONVERT_SCHEMA);

		var format_names = new Gtk.StringList (null);
		var default_format = settings.get_string (PREF_CONVERT_FORMAT);
		var selected_idx = 0;
		var idx = 0;
		saver_preferences = app.get_ordered_savers ();
		foreach (unowned var preferences in saver_preferences) {
			format_names.append (preferences.get_display_name ());
			if (default_format == preferences.get_content_type ()) {
				selected_idx = idx;
			}
			idx++;
		}
		format.model = format_names;
		format.selected = selected_idx;

		destination.folder = window.get_current_vfs_folder_or_default ();
	}

	[GtkCallback]
	void on_cancel (Gtk.Button button) {
		close ();
	}

	[GtkCallback]
	void on_save (Gtk.Button button) {
		saved ();
	}

	public void save_preferences () {
		settings.set_string (PREF_CONVERT_FORMAT, get_format () ?? "");
	}

	public FileOperation get_operation () {
		var file_operation = new ImageFileOperation (null);
		file_operation.content_type = get_format ();
		file_operation.folder = destination.folder;
		return file_operation;
	}

	unowned string? get_format () {
		var saver = saver_preferences[format.selected];
		return saver.get_content_type ();
	}

	GLib.Settings settings;
	GenericArray<SaverPreferences> saver_preferences;

	[GtkChild] unowned Gtk.DropDown format;
	[GtkChild] unowned Gth.FolderRow destination;
}
