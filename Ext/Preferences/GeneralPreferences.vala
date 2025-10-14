[GtkTemplate (ui = "/app/gthumb/gthumb/ui/general-preferences.ui")]
public class Gth.GeneralPreferences : Adw.NavigationPage {
	construct {
		settings = new GLib.Settings (GTHUMB_SCHEMA);
		terminal_settings = new GLib.Settings (GTHUMB_TERMINAL_SCHEMA);
		constructing = true;
		single_window_switch.active = settings.get_boolean (PREF_BROWSER_REUSE_ACTIVE_WINDOW);
		open_fullscreen_switch.active = settings.get_boolean (PREF_BROWSER_OPEN_IN_FULLSCREEN);
		terminal_command.text = terminal_settings.get_string (PREF_TERMINAL_COMMAND);
		constructing = false;
	}

	[GtkCallback]
	void on_single_window_activated (Object obj, ParamSpec param) {
		if (constructing) {
			return;
		}
		settings.set_boolean (PREF_BROWSER_REUSE_ACTIVE_WINDOW, single_window_switch.active);
	}

	[GtkCallback]
	void on_fullscreen_activated (Object obj, ParamSpec param) {
		if (constructing) {
			return;
		}
		settings.set_boolean (PREF_BROWSER_OPEN_IN_FULLSCREEN, open_fullscreen_switch.active);
	}

	[GtkCallback]
	void on_terminal_command_apply (Adw.EntryRow entry) {
		terminal_settings.set_string (PREF_TERMINAL_COMMAND, terminal_command.text);
	}

	[GtkChild] unowned Adw.SwitchRow open_fullscreen_switch;
	[GtkChild] unowned Adw.SwitchRow single_window_switch;
	[GtkChild] unowned Adw.EntryRow terminal_command;
	GLib.Settings settings;
	GLib.Settings terminal_settings;
	bool constructing;
}
