public class Gth.ReadText : Object {
	public string default_value;
	public bool is_filename;
	public CheckTextFunc check_func;

	string title;
	string save_label;

	public ReadText (string _title, string? _save_label = null) {
		title = _title;
		save_label = _save_label;
		default_value = null;
		is_filename = false; // TODO: select the name without the extension
		check_func = null;
	}

	public async string read_value (Gtk.Window? parent, Cancellable? cancellable = null) {
		callback = read_value.callback;
		dialog = new EntryDialog (title, default_value);
		dialog.check_func = check_func;
		if (save_label != null) {
			dialog.save_button.label = save_label;
		}
		dialog.saved.connect (() => {
			result = dialog.get_text ();
			dialog.close ();
		});
		dialog.closed.connect (() => {
			if (callback != null) {
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
		Util.after_timeout (200, () => {
			if (is_filename) {
				var ext_start = Util.get_extension_start (default_value);
				if (ext_start > 1) {
					dialog.entry.select_region (0, default_value.char_count (ext_start));
				}
			}
		});
		yield;
		if (cancelled_event != 0) {
			cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		return result;
	}

	SourceFunc callback = null;
	EntryDialog dialog = null;
	ulong cancelled_event = 0;
	string result = null;
}


[CCode (has_target = false)]
public delegate bool CheckTextFunc (string value) throws Error;


[GtkTemplate (ui = "/app/gthumb/gthumb/ui/entry-dialog.ui")]
public class Gth.EntryDialog : Adw.Dialog {
	public signal void saved ();

	public CheckTextFunc check_func;

	public EntryDialog (string _title, string? default_value = null) {
		title = _title;
		title_label.label = _title;
		check_func = null;
		if (default_value != null) {
			entry.text = default_value;
		}
	}

	public unowned string get_text () {
		return entry.text;
	}

	[GtkCallback]
	void on_cancel_clicked (Gtk.Button button) {
		close ();
	}

	[GtkCallback]
	void on_save_clicked (Gtk.Button button) {
		unowned var value = get_text ();
		try {
			if (Strings.empty (value)) {
				throw new IOError.FAILED (_("Enter a value"));
			}
			else if (check_func != null) {
				check_func (value);
			}
			saved ();
		}
		catch (Error error) {
			error_label.label = error.message;
			error_label.visible = true;
		}
	}

	[GtkChild] public unowned Gtk.Entry entry;
	[GtkChild] public unowned Gtk.Label title_label;
	[GtkChild] public unowned Gtk.Label error_label;
	[GtkChild] public unowned Gtk.Button save_button;
}
