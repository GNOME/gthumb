[GtkTemplate (ui = "/app/gthumb/gthumb/ui/preferences-video-page.ui")]
public class Gth.PreferencesVideoPage : Adw.NavigationPage {
	[GtkCallback]
	void on_select_folder (Adw.ActionRow row) {
		var initial_folder = Gth.Settings.get_file (settings, PREF_VIDEO_SCREESHOT_LOCATION);
		var dialog = new Gtk.FileDialog ();
		dialog.initial_folder = initial_folder;
		dialog.select_folder.begin (app.active_window, null, (obj, res) => {
			try {
				var folder = dialog.select_folder.end (res);
				settings.set_string (PREF_VIDEO_SCREESHOT_LOCATION, folder.get_uri ());
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

	construct {
		settings = new GLib.Settings (GTHUMB_VIDEO_SCHEMA);
		var initial_folder = Gth.Settings.get_file (settings, PREF_VIDEO_SCREESHOT_LOCATION);
		if (initial_folder == null) {
			initial_folder = Files.get_special_dir (UserDirectory.PICTURES);
		}
		update_screenshot_folder_name (initial_folder);
	}

	[GtkChild] unowned Adw.ActionRow screenshot_folder;
	[GtkChild] unowned Gtk.Image folder_icon;
	GLib.Settings settings;
}
