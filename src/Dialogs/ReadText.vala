public class Gth.ReadText : Object {
	public string default_value;
	public bool is_filename;
	public CheckTextFunc check_func;
	public TextGenerator generator;

	string title;
	string save_label;

	public ReadText (string _title, string? _save_label = null) {
		title = _title;
		save_label = _save_label;
		default_value = null;
		is_filename = false;
		check_func = null;
		generator = null;
	}

	public async string read_value (Gtk.Window? parent, Job job) throws Error {
		callback = read_value.callback;
		dialog = new EntryDialog (title, default_value);
		dialog.check_func = check_func;
		dialog.generator = generator;
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
		cancelled_event = job.cancellable.cancelled.connect (() => {
			dialog.close ();
		});
		job.opens_dialog ();
		dialog.present (parent);
		if (is_filename) {
			Util.after_timeout (200, () => Util.select_filename_without_ext (dialog.entry));
		}
		yield;
		job.dialog_closed ();
		if (cancelled_event != 0) {
			job.cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		if (result == null) {
			throw new IOError.CANCELLED ("Cancelled");
		}
		return result;
	}

	SourceFunc callback = null;
	EntryDialog dialog = null;
	ulong cancelled_event = 0;
	string result = null;
}


public delegate bool CheckTextFunc (string value) throws Error;
public delegate string GenerateTextFunc (string value) throws Error;

public class Gth.TextGenerator {
	public string tooltip;
	public string icon_name;
	public unowned GenerateTextFunc generate;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/entry-dialog.ui")]
public class Gth.EntryDialog : Adw.Dialog {
	public signal void saved ();

	public unowned CheckTextFunc check_func;
	public TextGenerator generator {
		set {
			_generator = value;
			if (_generator != null) {
				if (_generator.icon_name != null) {
					generator_button.icon_name = _generator.icon_name;
				}
				if (_generator.tooltip != null) {
					generator_button.tooltip_text = _generator.tooltip;
				}
				generator_button.visible = true;
			}
		}
	}

	public EntryDialog (string _title, string? default_value = null) {
		title = _title;
		check_func = null;
		if (default_value != null) {
			entry.text = default_value;
		}
	}

	public unowned string get_text () {
		return entry.text;
	}

	[GtkCallback]
	void on_generator_clicked (Gtk.Button button) {
		try {
			if (_generator == null) {
				throw new IOError.FAILED ("No Generator");
			}
			entry.text = _generator.generate (entry.text);
			entry.grab_focus ();
			Util.select_filename_without_ext (entry);
		}
		catch (Error error) {
			show_error (error.message);
		}
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
			show_error (error.message);
		}
	}

	void show_error (string msg) {
		error_label.label = msg;
		error_label.visible = true;
		entry.grab_focus ();
	}

	[GtkChild] public unowned Adw.EntryRow entry;
	[GtkChild] public unowned Gtk.Label error_label;
	[GtkChild] public unowned Gtk.Button save_button;
	[GtkChild] public unowned Gtk.Button generator_button;
	TextGenerator _generator = null;
}
