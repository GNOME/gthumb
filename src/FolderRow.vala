[GtkTemplate (ui = "/org/gnome/gthumb/ui/folder-row.ui")]
public class Gth.FolderRow : Adw.ActionRow {
	public File folder {
		get { return _folder; }
		set {
			_folder = value;
			if (_folder != null) {
				var vfs = new FileSourceVfs ();
				var info = vfs.get_display_info (_folder);
				folder_name.label = info.get_display_name ();
				folder_icon.set_from_gicon (info.get_symbolic_icon ());
				folder_icon.visible = true;
			}
			else {
				folder_name.label = "...";
				folder_icon.visible = false;
			}
		}
	}

	[GtkCallback]
	void on_select_folder (Gtk.Button _button) {
		var dialog = new Gtk.FileDialog ();
		dialog.initial_folder = folder;
		dialog.select_folder.begin (app.active_window, null, (obj, res) => {
			try {
				folder = dialog.select_folder.end (res);
			}
			catch (Error error) {
			}
		});
	}

	[GtkChild] unowned Gtk.Label folder_name;
	[GtkChild] unowned Gtk.Image folder_icon;
	File _folder = null;
}
