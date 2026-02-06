[GtkTemplate (ui = "/app/gthumb/gthumb/ui/scripts-dialog.ui")]
public class Gth.ScriptsDialog : Adw.PreferencesDialog {
	public ScriptsDialog () {
		tool_list.bind_model (app.tools.entries.model, new_tool_row);
		script_list.bind_model (app.scripts.entries.model, new_script_row);

		var empty_row = new Adw.ActionRow ();
		empty_row.title = _("Empty");
		empty_row.sensitive = false;
		empty_row.halign = Gtk.Align.CENTER;
		script_list.set_placeholder (empty_row);
	}

	Gtk.Widget new_tool_row (Object item) {
		var tool = item as Gth.FileAction;

		var row = new Gth.FileActionRow (tool);
		row.move_to_row.connect ((source_row, target_row) => {
			move_tool_to_position (source_row, target_row.get_index ());
		});
		row.move_to_top.connect ((source_row) => {
			move_tool_to_position (source_row, 0);
		});
		row.move_to_bottom.connect ((source_row) => {
			var last_pos = (int) app.tools.entries.model.get_n_items () - 1;
			move_tool_to_position (source_row, last_pos);
		});
		row.visibility_switch.notify["active"].connect ((_obj, _prop) => {
			var local_switch = _obj as Gtk.Switch;
			tool.visible = local_switch.active;
			app.tools.changed ();
		});

		return row;
	}

	Gtk.Widget new_script_row (Object item) {
		var script = item as Gth.Script;

		var row = new Gth.ScriptRow (script);
		row.activated.connect (() => {
			current_script = script;
			script_page.set_script (current_script);
			push_subpage (script_page);
		});
		row.move_to_row.connect ((source_row, target_row) => {
			move_script_to_position (source_row, target_row.get_index ());
		});
		row.move_to_top.connect ((source_row) => {
			move_script_to_position (source_row, 0);
		});
		row.move_to_bottom.connect ((source_row) => {
			var last_pos = (int) app.scripts.entries.model.get_n_items () - 1;
			move_script_to_position (source_row, last_pos);
		});
		row.delete_row.connect ((source_row) => {
			if (app.shortcuts.remove_detailed_action (script.detailed_action)) {
				app.shortcuts.changed ();
			}
			app.scripts.entries.remove (script);
			app.scripts.changed (script.id);
		});
		row.visibility_switch.notify["active"].connect ((_obj, _prop) => {
			var local_switch = _obj as Gtk.Switch;
			script.visible = local_switch.active;
			app.scripts.changed (script.id);
		});

		return row;
	}

	[GtkCallback]
	private void on_save_script (Gth.ScriptEditorPage source) {
		if (save_script ()) {
			pop_subpage ();
		}
	}

	[GtkCallback]
	private void on_add_script (Gtk.Button source) {
		current_script = null;
		script_page.set_script (current_script);
		push_subpage (script_page);
	}

	bool save_script () {
		try {
			var script = script_page.get_script ();
			if (current_script == null) {
				var shortcut = script_page.shortcut_row.get_shortcut ();
				shortcut.description = script.display_name;
				app.shortcuts.add (shortcut);
				app.scripts.entries.model.append (script);
				app.scripts.changed ();
			}
			else {
				current_script.copy (script);
				app.scripts.changed (script.id);
			}
			script_page.shortcut_row.save_if_modified ();
			return true;
		}
		catch (Error error) {
			add_toast (Util.new_literal_toast (error.message));
			return false;
		}
	}

	void move_script_to_position (Gth.ScriptRow row, int target_pos) {
		var source_pos = row.get_index ();
		if ((target_pos >= 0) && (target_pos != source_pos)) {
			app.scripts.entries.model.remove (source_pos);
			app.scripts.entries.model.insert (target_pos, row.script);
			app.scripts.changed ();
		}
	}

	void move_tool_to_position (Gth.FileActionRow row, int target_pos) {
		var source_pos = row.get_index ();
		if ((target_pos >= 0) && (target_pos != source_pos)) {
			app.tools.entries.model.remove (source_pos);
			app.tools.entries.model.insert (target_pos, row.tool);
			app.tools.changed ();
		}
	}

	Gth.Script current_script;
	[GtkChild] unowned Gtk.ListBox tool_list;
	[GtkChild] unowned Gtk.ListBox script_list;
	[GtkChild] unowned Gth.ScriptEditorPage script_page;
}
