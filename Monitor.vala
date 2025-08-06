public class Gth.Monitor {
	public enum Event {
		CREATED,
		DELETED,
		UPDATED,
	}

	public signal void bookmarks_changed () {
		app.foreach_window ((win) => win.browser.update_bookmarks_menu ());
	}

	public signal void catalog_saved (Gth.Catalog catalog, File old_file) {
		app.foreach_window ((win) => win.browser.catalog_saved (catalog, old_file));
	}

	// public signal void metadata_changed (Gth.FileData file);

	public signal void file_changed (File file, Event event);
}
