[GtkTemplate (ui = "/app/gthumb/gthumb/ui/video-preferences.ui")]
public class Gth.VideoPreferences : Adw.NavigationPage {
	[GtkCallback]
	void on_select_folder (Adw.ActionRow row) {
		var initial_folder = Gth.Settings.get_file (settings, PREF_VIDEO_SCREENSHOT_LOCATION);
		var dialog = new Gtk.FileDialog ();
		dialog.initial_folder = initial_folder;
		dialog.select_folder.begin (app.active_window, null, (obj, res) => {
			try {
				var folder = dialog.select_folder.end (res);
				settings.set_string (PREF_VIDEO_SCREENSHOT_LOCATION, folder.get_uri ());
				update_screenshot_folder_name (folder);
			}
			catch (Error error) {
			}
		});
	}

	void update_screenshot_folder_name (File? folder) {
		if (folder != null) {
			var vfs = new FileSourceVfs ();
			var info = vfs.get_display_info (folder);
			screenshot_folder.title = info.get_display_name ();
			folder_icon.set_from_gicon (info.get_symbolic_icon ());
			folder_icon.visible = true;
		}
		else {
			screenshot_folder.title = "";
			folder_icon.visible = false;
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
		var initial_folder = Gth.Settings.get_file (settings, PREF_VIDEO_SCREENSHOT_LOCATION);
		if (initial_folder == null) {
			initial_folder = Files.get_special_dir (UserDirectory.PICTURES);
		}
		update_screenshot_folder_name (initial_folder);
		scroll_action.selected = get_action_index (settings.get_enum (PREF_VIDEO_SCROLL_ACTION));
		constructing = false;
	}

	[GtkChild] unowned Adw.ActionRow screenshot_folder;
	[GtkChild] unowned Gtk.Image folder_icon;
	[GtkChild] unowned Adw.ComboRow scroll_action;
	GLib.Settings settings;
	bool constructing;

	const ScrollAction[] ACTION = {
		ScrollAction.CHANGE_CURRENT_TIME,
		ScrollAction.CHANGE_VOLUME,
		ScrollAction.CHANGE_FILE,
	};
}
