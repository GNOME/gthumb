[GtkTemplate (ui = "/org/gnome/gthumb/ui/saver-preferences-page.ui")]
public class Gth.SaverPreferencesPage : Adw.NavigationPage {
	public SaverPreferencesPage (SaverPreferences _options) {
		options = _options;
		title = options.get_display_name ();
		var preferences_page = options.create_widget ();
		toolbar_view.set_content (preferences_page);
	}

	SaverPreferences options;
	[GtkChild] unowned Adw.ToolbarView toolbar_view;
}
