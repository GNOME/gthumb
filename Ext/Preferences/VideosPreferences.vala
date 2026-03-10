[GtkTemplate (ui = "/app/gthumb/gthumb/ui/videos-preferences.ui")]
public class Gth.VideosPreferences : Adw.NavigationPage {
	[GtkCallback]
	void on_screenshot_folder_changed (Object obj, ParamSpec param) {
		if (constructing) {
			return;
		}
		if (screenshot_folder.folder != null) {
			settings.set_string (PREF_VIDEO_SCREENSHOT_LOCATION, screenshot_folder.folder.get_uri ());
		}
	}

	[GtkCallback]
	void on_screenshot_format_selected (Object obj, ParamSpec param) {
		if (constructing) {
			return;
		}
		var saver = saver_preferences[screenshot_format.selected];
		if (saver != null) {
			settings.set_string (PREF_VIDEO_SCREENSHOT_TYPE, saver.get_content_type ());
		}
	}

	[GtkCallback]
	void on_scroll_action_selected (Object obj, ParamSpec param) {
		if (constructing) {
			return;
		}
		var idx = scroll_action.selected;
		settings.set_enum (PREF_VIDEO_SCROLL_ACTION, ACTION[idx]);
	}

	[GtkCallback]
	void on_autorotate_activated (Object obj, ParamSpec param) {
		if (constructing) {
			return;
		}
		settings.set_boolean (PREF_VIDEO_AUTOROTATE, autorotate.active);
	}

	uint get_action_index (ScrollAction value) {
		uint idx = 0;
		foreach (unowned var item in ACTION) {
			if (value == item) {
				return idx;
			}
			idx++;
		}
		return 0;
	}

	construct {
		settings = new GLib.Settings (GTHUMB_VIDEOS_SCHEMA);
		constructing = true;

		scroll_action.selected = get_action_index (settings.get_enum (PREF_VIDEO_SCROLL_ACTION));
		autorotate.active = settings.get_boolean (PREF_VIDEO_AUTOROTATE);

		// Screenshot folder
		var folder = Gth.Settings.get_file (settings, PREF_VIDEO_SCREENSHOT_LOCATION);
		if (folder == null) {
			folder = Files.get_special_dir (UserDirectory.PICTURES);
		}
		screenshot_folder.folder = folder;

		// Screenshot format
		var selected_format = settings.get_string (PREF_VIDEO_SCREENSHOT_TYPE);
		var selected_idx = 0;
		var format_names = new Gtk.StringList (null);
		var idx = 0;
		saver_preferences = app.get_ordered_savers ();
		foreach (unowned var preferences in saver_preferences) {
			format_names.append (preferences.get_display_name ());
			if (preferences.get_content_type () == selected_format) {
				selected_idx = idx;
			}
			idx++;
		}
		screenshot_format.model = format_names;
		screenshot_format.selected = selected_idx;

		constructing = false;
	}

	[GtkChild] unowned Gth.FolderRow screenshot_folder;
	[GtkChild] unowned Adw.ComboRow screenshot_format;
	[GtkChild] unowned Adw.ComboRow scroll_action;
	[GtkChild] unowned Adw.SwitchRow autorotate;
	GLib.Settings settings;
	bool constructing;
	GenericArray<SaverPreferences> saver_preferences;

	const ScrollAction[] ACTION = {
		ScrollAction.CHANGE_CURRENT_TIME,
		ScrollAction.CHANGE_VOLUME,
		ScrollAction.CHANGE_FILE,
	};
}
