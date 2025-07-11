[GtkTemplate (ui = "/app/gthumb/gthumb/ui/bookmarks-dialog.ui")]
public class Gth.BookmarksDialog : Adw.PreferencesDialog {
	public BookmarksDialog () {
		bookmark_list.bind_model (app.bookmarks.entries.model, new_bookmark_row);
	}

	Gtk.Widget new_bookmark_row (Object item) {
		var entry = item as Gth.BookmarkEntry;
		var row = new Gth.BookmarkRow (entry);
		row.edit_button.clicked.connect (() => {
			current_entry = entry;
			uri_row.text = entry.file.get_uri ();
			name_row.text = entry.display_name;
			push_subpage (entry_page);
		});
		row.delete_button.clicked.connect (() => {
			delete_entry.begin (entry);
		});
		row.move_to_row.connect ((source_row, target_row) => {
			move_row_to_position.begin (source_row, target_row.get_index ());
		});
		return row;
	}

	[GtkCallback]
	void on_save (Gtk.Button button) {
		save.begin ();
	}

	async void move_row_to_position (BookmarkRow row, int target_pos) {
		var source_pos = row.get_index ();
		if ((target_pos < 0) || (target_pos == source_pos))
			return;
		try {
			app.bookmarks.entries.model.remove (source_pos);
			app.bookmarks.entries.model.insert (target_pos, row.entry);

			app.bookmarks.menu.remove (source_pos);
			var menu_item = Bookmarks.new_menu_item_from_entry (row.entry);
			app.bookmarks.menu.insert_item (target_pos, menu_item);

			yield app.bookmarks.save_app_bookmarks ();
		}
		catch (Error error) {
			add_toast (new Adw.Toast (error.message));
		}
	}

	async void delete_entry (BookmarkEntry entry) {
		try {
			var pos = app.bookmarks.entries.find (entry);
			if (pos < 0)
				return;
			app.bookmarks.entries.model.remove (pos);
			app.bookmarks.menu.remove (pos);
			yield app.bookmarks.save_app_bookmarks ();
		}
		catch (Error error) {
			add_toast (new Adw.Toast (error.message));
		}
	}

	async void save () {
		try {
			if (Strings.empty (uri_row.text)) {
				throw new IOError.FAILED (_("Invalid location"));
			}
			if (Strings.empty (name_row.text)) {
				throw new IOError.FAILED (_("Invalid name"));
			}
			current_entry.file = File.new_for_uri (uri_row.text);
			current_entry.display_name = name_row.text;
			var pos = app.bookmarks.entries.find (current_entry);
			if (pos < 0)
				return;
			app.bookmarks.entries.model.items_changed (pos, 1, 1);

			app.bookmarks.menu.remove (pos);
			var menu_item = Bookmarks.new_menu_item_from_entry (current_entry);
			app.bookmarks.menu.insert_item (pos, menu_item);

			yield app.bookmarks.save_app_bookmarks ();
			pop_subpage ();
		}
		catch (Error error) {
			add_toast (new Adw.Toast (error.message));
		}
	}

	[GtkChild] unowned Gtk.ListBox bookmark_list;
	[GtkChild] unowned Adw.EntryRow uri_row;
	[GtkChild] unowned Adw.EntryRow name_row;
	[GtkChild] unowned Adw.NavigationPage entry_page;

	BookmarkEntry current_entry;
}
