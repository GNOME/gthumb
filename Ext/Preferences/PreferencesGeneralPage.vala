[GtkTemplate (ui = "/app/gthumb/gthumb/ui/preferences-general-page.ui")]
public class Gth.PreferencesGeneralPage : Adw.NavigationPage {
	construct {
		settings = new GLib.Settings (GTHUMB_SCHEMA);
		constructing = true;
		open_fullscreen_switch.active = settings.get_boolean (PREF_BROWSER_OPEN_IN_FULLSCREEN);
		constructing = false;
	}

	[GtkCallback]
	void on_fullscreen_activated (Object obj, ParamSpec param) {
		if (constructing) {
			return;
		}
		settings.set_boolean (PREF_BROWSER_OPEN_IN_FULLSCREEN, open_fullscreen_switch.active);
	}

	[GtkChild] unowned Adw.SwitchRow open_fullscreen_switch;
	GLib.Settings settings;
	bool constructing;
}
