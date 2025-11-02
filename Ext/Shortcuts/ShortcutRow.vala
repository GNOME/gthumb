[GtkTemplate (ui = "/app/gthumb/gthumb/ui/shortcut-row.ui")]
public class Gth.ShortcutRow : Adw.ActionRow {
	public bool save_on_change { get; set; default = true; }

	public void set_shortcut (Shortcut? _shortcut, string? title = null) {
		if ((shortcut != null) && (shortcut_changed_id != 0)) {
			shortcut.disconnect (shortcut_changed_id);
			shortcut = null;
			shortcut_changed_id = 0;
		}

		if (_shortcut == null) {
			return;
		}

		shortcut = _shortcut;
		row_shortcut.default_accelerator = shortcut.default_accelerator;
		row_shortcut.set_accelerator (shortcut.accelerator);

		shortcut_changed_id = shortcut.changed.connect ((obj) => {
			var local_shortcut = obj as Shortcut;
			row_shortcut.set_accelerator (local_shortcut.accelerator);
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
	}

	public Shortcut get_shortcut () {
		return shortcut;
	}

	public void save_if_modified () {
		if (shortcut == null) {
			return;
		}
		if (row_shortcut.accelerator != shortcut.accelerator) {
			save ();
		}
	}

	void update_label () {
		if (row_shortcut.accelerator_label != null) {
			accel_label.set_label (row_shortcut.accelerator_label);
			if (!row_shortcut.get_is_modified ()) {
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

	inline void after_changing_shortcut () {
		if (save_on_change) {
			save ();
		}
	}

	void save () {
		// If another shortcut has the same accelerator,
		// reset the accelerator for that shortcut.
		var other_shortcut = app.shortcuts.find_by_key (shortcut.context,
			row_shortcut.keyval, row_shortcut.modifiers, shortcut);
		if (other_shortcut != null) {
			other_shortcut.remove_key ();
		}
		shortcut.set_accelerator (row_shortcut.accelerator);
		app.shortcuts.changed ();
	}

	construct {
		row_shortcut = new Shortcut ("row.shortcut");
		row_shortcut.changed.connect (() => {
			update_label ();
		});

		activated.connect (() => {
			var dialog = new ReadShortcut ();
			var win = app.get_active_main_window ();
			if (win == null) {
				return;
			}
			var job = win.new_job ("Read Shortcut");
			dialog.read_accelerator.begin (win, job, (_obj, res) => {
				try {
					var info = dialog.read_accelerator.end (res);
					row_shortcut.set_key (info);
					after_changing_shortcut ();
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

		var action = new SimpleAction ("reset", null);
		action.activate.connect ((_action, param) => {
			row_shortcut.set_accelerator (shortcut.default_accelerator);
			after_changing_shortcut ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("delete", null);
		action.activate.connect ((_action, param) => {
			row_shortcut.set_accelerator (null);
			after_changing_shortcut ();
		});
		action_group.add_action (action);
	}

	[GtkChild] unowned Gtk.Label accel_label;
	Shortcut row_shortcut;
	Shortcut shortcut;
	ulong shortcut_changed_id;
}
