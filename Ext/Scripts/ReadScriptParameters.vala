public class Gth.ReadScriptParameters : Object {
	public FileData file = null;

	public async void ask_user (Gth.Window parent, string title, GenericArray<ScriptParameter> parameters, bool can_skip, Job job) throws Error {
		callback = ask_user.callback;
		dialog = new ScriptParametersDialog (parent, title, parameters, file);
		dialog.skip_button.visible = can_skip;
		dialog.saved.connect (() => {
			run_script = true;
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
		yield;
		job.dialog_closed ();
		dialog.cancel_thumbnailer ();
		if (cancelled_event != 0) {
			job.cancellable.disconnect (cancelled_event);
			cancelled_event = 0;
		}
		if (dialog.skipped) {
			throw new ScriptError.SKIPPED ("Skipped");
		}
		if (!run_script) {
			throw new IOError.CANCELLED ("Cancelled");
		}
	}

	SourceFunc callback = null;
	ScriptParametersDialog dialog = null;
	ulong cancelled_event = 0;
	bool run_script = false;
}


[GtkTemplate (ui = "/app/gthumb/gthumb/ui/script-parameters-dialog.ui")]
class Gth.ScriptParametersDialog : Adw.Dialog {
	public signal void saved ();
	public bool skipped;

	public ScriptParametersDialog (Gth.Window parent, string title_text, GenericArray<ScriptParameter> _parameters, FileData? file) {
		custom_title.label = title_text;
		file_info.visible = file != null;
		if (file != null) {
			filename.label = file.get_display_name ();

			thumbnail.bind (file);
			thumbnail.size = parent.browser.thumbnail_size;

			thumbnailer = new Thumbnailer (parent);
			thumbnailer.requested_size = parent.browser.thumbnail_size;
			thumbnailer.add (file);
		}
		else {
			thumbnailer = null;
		}
		skipped = false;
		parameters = _parameters;
		entries = new GenericArray<weak Adw.EntryRow>();
		foreach (var parameter in parameters) {
			var entry = new Adw.EntryRow ();
			entry.title = parameter.title;
			entry.text = parameter.default_value;
			entry_group.add (entry);
			entries.add (entry);
		}
	}

	public void cancel_thumbnailer () {
		thumbnail.unbind ();
		if (thumbnailer != null) {
			thumbnailer.cancel ();
		}
	}

	[GtkCallback]
	void on_skip_clicked (Gtk.Button button) {
		skipped = true;
		close ();
	}

	[GtkCallback]
	void on_run_clicked (Gtk.Button button) {
		for (var i = 0; i < parameters.length; i++) {
			var param = parameters[i];
			var entry = entries[i];
			param.value = entry.text;
		}
		saved ();
	}

	[GtkChild] unowned Gtk.Label custom_title;
	[GtkChild] unowned Gtk.Label filename;
	[GtkChild] public unowned Gtk.Button skip_button;
	[GtkChild] unowned Adw.PreferencesGroup entry_group;
	[GtkChild] unowned Gtk.Box file_info;
	[GtkChild] unowned Gth.Thumbnail thumbnail;
	Thumbnailer thumbnailer;
	GenericArray<ScriptParameter> parameters;
	GenericArray<weak Adw.EntryRow> entries;
}
