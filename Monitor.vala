public class Gth.Monitor {
	public enum Event {
		CREATED,
		DELETED,
		UPDATED,
	}

	public signal void bookmarks_changed () {
		app.foreach_window ((win) => win.browser.update_bookmarks_menu ());
	}

	public signal void file_renamed (File old_file, File new_file);

	public signal void metadata_changed (Gth.FileData file);

	public signal void file_changed (File file, Event event);
}
