public class Gth.Monitor {
	public signal void bookmarks_changed () {
		app.foreach_window ((win) => win.update_bookmarks_menu ());
	}
	public signal void file_renamed (File old_file, File new_file);
	public signal void metadata_changed (Gth.FileData file);
}
