[GtkTemplate (ui = "/app/gthumb/gthumb/ui/bookmarks-dialog.ui")]
public class Gth.BookmarksDialog : Adw.PreferencesDialog {
	public BookmarksDialog () {
		var empty_row = new Adw.ActionRow ();
		empty_row.title = _("Empty");
		empty_row.sensitive = false;
		empty_row.halign = Gtk.Align.CENTER;
		bookmark_list.set_placeholder (empty_row);

		bookmark_list.bind_model (app.bookmarks.app_bookmarks.model, new_bookmark_row);
	}

	Gtk.Widget new_bookmark_row (Object item) {
		var entry = item as Gth.ActionInfo;
		var row = new Gth.BookmarkRow (entry);
		row.edit_button.clicked.connect (() => {
			current_entry = entry;
			uri_row.text = entry.value.get_string ();
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
			app.bookmarks.app_bookmarks.model.remove (source_pos);
			app.bookmarks.app_bookmarks.model.insert (target_pos, row.entry);
			yield app.bookmarks.save_app_bookmarks ();
		}
		catch (Error error) {
			add_toast (Util.new_literal_toast (error.message));
		}
	}

	async void delete_entry (ActionInfo entry) {
		try {
			var pos = app.bookmarks.app_bookmarks.find (entry);
			if (pos < 0) {
				return;
			}
			app.bookmarks.app_bookmarks.model.remove (pos);
			yield app.bookmarks.save_app_bookmarks ();
		}
		catch (Error error) {
			add_toast (Util.new_literal_toast (error.message));
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
			current_entry.value = new Variant.string (uri_row.text);
			current_entry.display_name = name_row.text;
			var pos = app.bookmarks.app_bookmarks.find (current_entry);
			if (pos < 0) {
				return;
			}
			app.bookmarks.app_bookmarks.model.items_changed (pos, 1, 1);

			yield app.bookmarks.save_app_bookmarks ();
			pop_subpage ();
		}
		catch (Error error) {
			add_toast (Util.new_literal_toast (error.message));
		}
	}

	[GtkChild] unowned Gtk.ListBox bookmark_list;
	[GtkChild] unowned Adw.EntryRow uri_row;
	[GtkChild] unowned Adw.EntryRow name_row;
	[GtkChild] unowned Adw.NavigationPage entry_page;

	ActionInfo current_entry;
}
