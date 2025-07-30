[GtkTemplate (ui = "/app/gthumb/gthumb/ui/preferences-browser-page.ui")]
public class Gth.PreferencesBrowserPage : Adw.NavigationPage {
	construct {
		settings = new GLib.Settings (GTHUMB_SCHEMA);
		thumbnail_size_adjustment.set_value (settings.get_int (PREF_BROWSER_THUMBNAIL_SIZE));
	}

	[GtkCallback]
	void on_thumbnail_size_changed (Gtk.Adjustment adj) {
		settings.set_int (PREF_BROWSER_THUMBNAIL_SIZE, (int) adj.get_value ());
	}

	[GtkCallback]
	bool on_thumbnail_size_change_value (Gtk.Range range, Gtk.ScrollType scroll, double new_value) {
		var int_value = (int) (Math.round (new_value / THUMBNAIL_SIZE_STEP) * THUMBNAIL_SIZE_STEP);
		range.set_value (int_value);
		return true;
	}

	[GtkChild] unowned Gtk.Adjustment thumbnail_size_adjustment;
	GLib.Settings settings;
	const double THUMBNAIL_SIZE_STEP = 32.0;
}
