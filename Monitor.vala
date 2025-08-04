public class Gth.Monitor {
	public signal void bookmarks_changed () {
		app.foreach_window ((win) => win.browser.update_bookmarks_menu ());
	}
	public signal void file_renamed (File old_file, File new_file);
	public signal void metadata_changed (Gth.FileData file);
	public signal void directory_created (File file);
	public signal void file_created (File folder, File file, Window ignore_window = null) {
		app.foreach_window ((win) => {
			if (win != ignore_window) {
				win.browser.file_created (folder, file);
			}
		});
	}
}
