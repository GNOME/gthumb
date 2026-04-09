[GtkTemplate (ui = "/org/gnome/gthumb/ui/general-preferences.ui")]
public class Gth.GeneralPreferences : Adw.NavigationPage {
	[GtkCallback]
	void on_restore_session_changed (Object obj, ParamSpec param) {
		if (constructing) {
			return;
		}
		settings.set_boolean (PREF_BROWSER_RESTORE_SESSION, restore_session_check.active);
	}

	[GtkCallback]
	void on_single_window_activated (Object obj, ParamSpec param) {
		if (constructing) {
			return;
		}
		settings.set_boolean (PREF_BROWSER_REUSE_ACTIVE_WINDOW, single_window.active);
	}

	[GtkCallback]
	void on_terminal_command_apply (Adw.EntryRow entry) {
		terminal_settings.set_string (PREF_TERMINAL_COMMAND, terminal_command.text);
	}

	[GtkCallback]
	void on_home_folder_changed (Object obj, ParamSpec param) {
		if (constructing) {
			return;
		}
		if (home_folder.folder != null) {
			settings.set_string (PREF_BROWSER_HOME_FOLDER, home_folder.folder.get_uri ());
		}
	}

	construct {
		settings = new GLib.Settings (GTHUMB_SCHEMA);
		terminal_settings = new GLib.Settings (GTHUMB_TERMINAL_SCHEMA);
		constructing = true;
		if (settings.get_boolean (PREF_BROWSER_RESTORE_SESSION)) {
			restore_session_check.active = true;
		}
		else {
			ignore_session_check.active = true;
		}
		single_window.active = settings.get_boolean (PREF_BROWSER_REUSE_ACTIVE_WINDOW);
		terminal_command.text = terminal_settings.get_string (PREF_TERMINAL_COMMAND);
		home_folder.folder = Gth.Settings.get_home_folder (settings);
		constructing = false;
	}

	[GtkChild] unowned Gtk.CheckButton restore_session_check;
	[GtkChild] unowned Gtk.CheckButton ignore_session_check;
	[GtkChild] unowned Adw.SwitchRow single_window;
	[GtkChild] unowned Adw.EntryRow terminal_command;
	[GtkChild] unowned Gth.FolderRow home_folder;
	GLib.Settings settings;
	GLib.Settings terminal_settings;
	bool constructing;
}
