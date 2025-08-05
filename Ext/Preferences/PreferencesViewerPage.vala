[GtkTemplate (ui = "/app/gthumb/gthumb/ui/preferences-viewer-page.ui")]
public class Gth.PreferencesViewerPage : Adw.NavigationPage {
	construct {
		settings = new GLib.Settings (GTHUMB_VIEWER_SCHEMA);
		constructing = true;
		// TODO
		constructing = false;
	}

	GLib.Settings settings;
	bool constructing;
}
