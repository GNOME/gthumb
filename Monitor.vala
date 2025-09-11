public class Gth.Monitor : Object {
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

	public void metadata_changed (Gth.FileData file_data) {
		app.foreach_window ((win) => win.metadata_changed (file_data));
	}

	public signal void file_changed (File file, Event event);

	public signal void files_deleted (GenericList<File> files) {
		app.foreach_window ((win) => win.browser.files_deleted (files));
	}

	public signal void files_created (File parent, GenericList<File> files) {
		app.foreach_window ((win) => win.browser.files_created (parent, files));
	}

	public void file_created (File file) {
		var parent = file.get_parent ();
		if (parent == null) {
			return;
		}
		var files = new GenericList<File>();
		files.model.append (file);
		files_created (parent, files);
	}

	public void file_deleted (File file) {
		var files = new GenericList<File>();
		files.model.append (file);
		files_deleted (files);
	}
}
