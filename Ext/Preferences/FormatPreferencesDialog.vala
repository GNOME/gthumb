public class Gth.FormatPreferencesDialog : Object {
	public async void change_options (Gtk.Window? parent, string content_type, Job job) throws Error {
		callback = change_options.callback;
		dialog = new FormatDialog (content_type);
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
	FormatDialog dialog = null;
	ulong cancelled_event = 0;
	bool saved = false;
}

[GtkTemplate (ui = "/app/gthumb/gthumb/ui/format-dialog.ui")]
class Gth.FormatDialog : Adw.Dialog {
	public signal void saved ();

	public FormatDialog (string content_type) {
		settings = new GLib.Settings (GTHUMB_SCHEMA);

		preferences = app.get_saver_preferences (content_type);
		// Translators: %s is a file format, such as JPEG
		title = _("%s Options").printf (preferences.get_display_name ());

		//var show_options = new Adw.SwitchRow ();
		//show_options.title = _("Show this dialog before saving");
		//
		var show_options = new Gtk.CheckButton.with_label (_("Show the options before saving"));
		show_options.active = settings.get_boolean (PREF_GENERAL_SHOW_FORMAT_OPTIONS);
		show_options.notify["active"].connect ((obj, _param) => {
			var local_switch = obj as Gtk.CheckButton;
			settings.set_boolean (PREF_GENERAL_SHOW_FORMAT_OPTIONS, local_switch.active);
		});

		var group = new Adw.PreferencesGroup ();
		group.add (show_options);

		var page = preferences.create_widget (true);
		page.add (group);
		main_view.content = page;
	}

	[GtkCallback]
	void on_cancel (Gtk.Button button) {
		close ();
	}

	[GtkCallback]
	void on_save (Gtk.Button button) {
		saved ();
	}

	SaverPreferences preferences;
	GLib.Settings settings;
	[GtkChild] unowned Adw.ToolbarView main_view;
}
