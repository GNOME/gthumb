[GtkTemplate (ui = "/app/gthumb/gthumb/ui/preferences-saving-page.ui")]
public class Gth.PreferencesSavingPage : Adw.NavigationPage {
	construct {
		saver_preferences = new GenericArray<SaverPreferences> ();
		foreach (unowned var type in app.saver_preferences.get_values ()) {
			var preferences = Object.new (type) as Gth.SaverPreferences;
			if (preferences != null) {
				saver_preferences.add (preferences);
			}
		}

		saver_preferences.sort ((a, b) => strcmp (a.get_display_name (), b.get_display_name ()));
		foreach (unowned var preferences in saver_preferences) {
			var row = new Adw.ActionRow ();
			row.title = preferences.get_display_name ();
			row.add_suffix (new Gtk.Image.from_icon_name ("right-symbolic"));
			row.activatable = true;
			type_list.append (row);
		}
	}

	[GtkCallback]
	void on_row_activated (Gtk.ListBoxRow row) {
		var idx = row.get_index ();
		var preferences = saver_preferences[idx] as SaverPreferences;
		if (preferences != null) {
			var navigation_view = Util.get_parent<Adw.NavigationView> (this);
			if (navigation_view != null) {
				var page = new Gth.SaverPreferencesPage (preferences);
				navigation_view.push (page);
			}
		}
	}

	GenericArray<SaverPreferences> saver_preferences;
	[GtkChild] unowned Gtk.ListBox type_list;
}
