[GtkTemplate (ui = "/org/gnome/gthumb/ui/script-editor-page.ui")]
public class Gth.ScriptEditorPage : Adw.NavigationPage {
	public Gth.Script script;

	public signal void save ();

	public void set_script (Gth.Script? current_script) {
		Shortcut shortcut = null;
		if (current_script != null) {
			script = current_script.duplicate ();
			shortcut = app.shortcuts.find_by_action (script.detailed_action);
		}
		else {
			script = new Gth.Script ();
			script.visible = true;
		}
		if (shortcut == null) {
			shortcut = script.create_shortcut ();
		}
		script.bind_property ("display_name", name_entry, "text", BindingFlags.SYNC_CREATE | BindingFlags.BIDIRECTIONAL);
		//script.bind_property ("command", command, "subtitle", BindingFlags.SYNC_CREATE);
		script.bind_property ("shell_script", shell_script, "active", BindingFlags.SYNC_CREATE | BindingFlags.BIDIRECTIONAL);
		script.bind_property ("for_each_file", for_each_file, "active", BindingFlags.SYNC_CREATE | BindingFlags.BIDIRECTIONAL);
		script.bind_property ("wait_command", wait_command, "active", BindingFlags.SYNC_CREATE | BindingFlags.BIDIRECTIONAL);
		command_preview.label = script.get_preview ();
		shortcut_row.set_shortcut (shortcut, _("Shortcut"));
	}

	public Script? get_script () throws Error {
		if (Strings.empty (name_entry.text)) {
			name_entry.grab_focus ();
			throw new IOError.FAILED (_("Name is empty"));
		}
		if (Strings.empty (script.command)) {
			command.grab_focus ();
			throw new IOError.FAILED (_("Command is empty"));
		}
		return script;
	}

	[GtkCallback]
	void on_save (Gtk.Button source) {
		save ();
	}

	[GtkCallback]
	void on_activated_command (Adw.ActionRow row) {
		command_page.set_template_text (script.command, TemplateFlags.NO_ENUMERATOR);
		Util.push_page (this, command_page);
	}

	[GtkCallback]
	private void on_save_command (Gth.TemplatePage source) {
		script.command = command_page.get_template ();
		command_preview.label = script.get_preview ();
		Util.pop_page (this);
	}

	[GtkChild] unowned Adw.EntryRow name_entry;
	[GtkChild] unowned Adw.ActionRow command;
	[GtkChild] unowned Gtk.Label command_preview;
	[GtkChild] unowned Adw.SwitchRow shell_script;
	[GtkChild] unowned Adw.SwitchRow for_each_file;
	[GtkChild] unowned Adw.SwitchRow wait_command;
	[GtkChild] unowned Gth.ScriptCommandPage command_page;
	[GtkChild] public unowned Gth.ShortcutRow shortcut_row;
}
