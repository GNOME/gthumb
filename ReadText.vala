public class Gth.ReadText : Object {
	public string default_value;
	string title;
	string save_label;

	public ReadText (string _title, string? _save_label = null) {
		title = _title;
		save_label = _save_label;
		default_value = null;
	}

	public async string read_value (Gtk.Window? parent, Cancellable? cancellable = null) {
		callback = read_value.callback;
		dialog = new EntryDialog (title, default_value);
		if (save_label != null) {
			dialog.save_button.label = save_label;
		}
		dialog.saved.connect (() => {
			result = dialog.get_text ();
			dialog.close ();
		});
		dialog.closed.connect (() => {
			if (callback != null) {
				Idle.add (callback);
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
		return result;
	}

	SourceFunc callback = null;
	EntryDialog dialog = null;
	ulong cancelled_event = 0;
	string result = null;
}


[GtkTemplate (ui = "/app/gthumb/gthumb/ui/entry-dialog.ui")]
public class Gth.EntryDialog : Adw.Dialog {
	public signal void saved ();

	public EntryDialog (string _title, string? default_value = null) {
		title = _title;
		title_label.label = _title;
		if (default_value != null) {
			entry.text = default_value;
		}
	}

	public string get_text () {
		return entry.text;
	}

	[GtkCallback]
	void on_cancel_clicked (Gtk.Button button) {
		close ();
	}

	[GtkCallback]
	void on_save_clicked (Gtk.Button button) {
		saved ();
	}

	[GtkChild] public unowned Gtk.Entry entry;
	[GtkChild] public unowned Gtk.Label title_label;
	[GtkChild] public unowned Gtk.Button save_button;
}
