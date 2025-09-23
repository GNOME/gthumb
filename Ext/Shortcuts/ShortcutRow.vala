[GtkTemplate (ui = "/app/gthumb/gthumb/ui/shortcut-row.ui")]
public class Gth.ShortcutRow : Adw.ActionRow {
	public void set_shortcut (Shortcut? _shortcut, string? title = null) {
		if (_shortcut == null) {
			return;
		}
		shortcut = _shortcut;
		shortcut.changed.connect (() => {
			update_label ();
		});

		activatable = true;
		if (title != null) {
			set_title (title);
		}
		else if (!Strings.empty (shortcut.description)) {
			set_title (shortcut.description);
		}
		else {
			set_title (_("Shortcut"));
		}

		update_label ();

		activated.connect (() => {
			var dialog = new ReadShortcut ();
			var win = app.get_active_window ();
			if (win == null) {
				return;
			}
			var job = win.new_job ("Read Shortcut");
			dialog.read_accelerator.begin (win, job, (_obj, res) => {
				try {
					var info = dialog.read_accelerator.end (res);
					// If another shortcut has the same accelerator,
					// reset the accelerator for that shortcut.
					var other_shortcut = app.shortcuts.find_by_key (shortcut.context, info.keyval, info.state);
					if (other_shortcut != null) {
						other_shortcut.remove_key ();
					}
					shortcut.set_key (info);
					app.shortcuts.changed ();
				}
				catch (Error error) {
					win.show_error (error);
				}
				finally {
					job.done ();
				}
			});
		});

		var action_group = new SimpleActionGroup ();
		insert_action_group ("row", action_group);

		if (shortcut.default_accelerator != null) {
			var action = new SimpleAction ("reset", null);
			action.activate.connect ((_action, param) => {
				shortcut.set_accelerator (shortcut.default_accelerator);
			});
			action_group.add_action (action);
		}

		var action = new SimpleAction ("delete", null);
		action.activate.connect ((_action, param) => {
			shortcut.set_accelerator (null);
		});
		action_group.add_action (action);
	}

	void update_label () {
		if (shortcut.accelerator_label != null) {
			accel_label.set_label (shortcut.accelerator_label);
			if (!shortcut.get_is_modified ()) {
				accel_label.add_css_class ("dimmed");
			}
			else {
				accel_label.remove_css_class ("dimmed");
			}
		}
		else {
			accel_label.set_label ("");
		}
	}

	[GtkChild] unowned Gtk.Label accel_label;
	Shortcut shortcut;
}
