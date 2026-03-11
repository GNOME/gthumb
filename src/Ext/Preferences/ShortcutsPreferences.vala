[GtkTemplate (ui = "/app/gthumb/gthumb/ui/shortcuts-preferences.ui")]
public class Gth.ShortcutsPreferences : Adw.NavigationPage {
	GenericList<CategoryEntry> categories;

	construct {
		var action_group = new SimpleActionGroup ();
		insert_action_group ("shortcuts", action_group);

		var action = new SimpleAction ("set-category", GLib.VariantType.INT32);
		action.activate.connect ((_action, param) => {
			var idx = param.get_int32 ();
			var entry = categories.get (idx);
			var button = category_buttons[idx];
			if (entry == current_category) {
				current_category = null;
				button.active = false;
				last_category_button = null;
			}
			else {
				if (last_category_button != null) {
					last_category_button.active = false;
				}
				button.active = true;
				last_category_button = button;
				current_category = entry;
			}
			update_search ();
		});
		action_group.add_action (action);

		search_bar.connect_entry (search);
		sorted_shortcuts = app.shortcuts.entries.duplicate ();
		sorted_shortcuts.sort ((a, b) => Util.intcmp (a.category, b.category));

		categories = new GenericList<CategoryEntry> ();
		// Translators: filter that shows all the shortcuts.
		categories.model.append (new CategoryEntry.all (C_("Shortcuts", "All")));
		// Translators: filter that shows the modified shortcuts.
		categories.model.append (new CategoryEntry.modified (C_("Shortcuts", "Modified")));
		var last_category = -1;
		foreach (var shortcut in sorted_shortcuts) {
			if (!shortcut.get_is_customizable ()) {
				continue;
			}
			if (shortcut.category == ShortcutCategory.HIDDEN) {
				continue;
			}
			if (shortcut.category != last_category) {
				categories.model.append (new CategoryEntry.for_category (shortcut.category));
				last_category = shortcut.category;
			}
		}
		categories.sort (CategoryEntry.compare);

		category_buttons = new GenericArray<unowned Gtk.ToggleButton>();
		var idx = 0;
		foreach (var entry in categories) {
			var button = new Gtk.ToggleButton.with_label (entry.title);
			button.add_css_class ("category");
			button.action_name = "shortcuts.set-category";
			button.action_target = new Variant.int32 (idx);
			category_box.append (button);
			category_buttons.add (button);
			idx++;
		}
		current_category = null;

		update_list (CategoryEntry.Type.ALL);
	}

	GenericList<Shortcut> sorted_shortcuts;
	GenericArray<weak Adw.PreferencesGroup> groups = null;

	void update_list (CategoryEntry.Type filter_type, ShortcutCategory category = ShortcutCategory.HIDDEN) {
		if (groups != null) {
			for (var idx = 0; idx < groups.length; idx++) {
				shortcut_page.remove (groups[idx]);
			}
			groups.length = 0;
		}
		string text_filter = search_bar.search_mode_enabled ? search.text.down () : null;
		groups = new GenericArray<weak Adw.PreferencesGroup>();
		Adw.PreferencesGroup group = null;
		Gtk.ListBox shortcut_list = null;
		switch (filter_type) {
		case CategoryEntry.Type.CATEGORY:
			shortcut_list = new_shortcut_list ();
			group = new Adw.PreferencesGroup ();
			group.add (shortcut_list);
			shortcut_page.add (group);
			groups.add (group);
			foreach (var shortcut in sorted_shortcuts) {
				if (!shortcut.get_is_customizable ()) {
					continue;
				}
				if (shortcut.category != category) {
					continue;
				}
				if (!shortcut.match_filter (text_filter)) {
					continue;
				}
				shortcut_list.append (new_shortcut_row (shortcut));
			}
			break;

		case CategoryEntry.Type.MODIFIED:
			shortcut_list = new_shortcut_list ();
			group = new Adw.PreferencesGroup ();
			group.add (shortcut_list);
			shortcut_page.add (group);
			groups.add (group);
			foreach (var shortcut in sorted_shortcuts) {
				if (!shortcut.get_is_customizable ()) {
					continue;
				}
				if (!shortcut.get_is_modified ()) {
					continue;
				}
				if (!shortcut.match_filter (text_filter)) {
					continue;
				}
				shortcut_list.append (new_shortcut_row (shortcut));
			}
			break;

		case CategoryEntry.Type.ALL:
			var last_category = -1;
			foreach (var shortcut in sorted_shortcuts) {
				if (!shortcut.get_is_customizable ()) {
					continue;
				}
				if (shortcut.category == ShortcutCategory.HIDDEN) {
					continue;
				}
				if (!shortcut.match_filter (text_filter)) {
					continue;
				}
				if (shortcut.category != last_category) {
					last_category = shortcut.category;
					shortcut_list = new_shortcut_list ();
					group = new Adw.PreferencesGroup ();
					group.title = _(shortcut.category.to_title ());
					group.add (shortcut_list);
					shortcut_page.add (group);
					groups.add (group);
				}
				shortcut_list.append (new_shortcut_row (shortcut));
			}
			break;
		}
	}

	Gtk.ListBox new_shortcut_list () {
		var shortcut_list = new Gtk.ListBox ();
		shortcut_list.add_css_class ("boxed-list");
		shortcut_list.selection_mode = Gtk.SelectionMode.NONE;
		return shortcut_list;
	}

	Gtk.Widget new_shortcut_row (Object item) {
		var row = new ShortcutRow ();
		row.set_shortcut (item as Shortcut);
		return row;
	}

	[GtkCallback]
	void on_search_changed (Gtk.SearchEntry _obj) {
		update_search ();
	}

	[GtkCallback]
	void on_search_mode_enabled (Object obj, ParamSpec param) {
		update_search ();
	}

	void update_search () {
		if (current_category != null) {
			update_list (current_category.type, current_category.category);
		}
		else {
			update_list (CategoryEntry.Type.ALL);
		}
	}

	[GtkChild] unowned Adw.PreferencesPage shortcut_page;
	[GtkChild] unowned Adw.WrapBox category_box;
	[GtkChild] unowned Gtk.SearchEntry search;
	[GtkChild] unowned Gtk.SearchBar search_bar;
	unowned Gtk.ToggleButton last_category_button;
	CategoryEntry current_category = null;
	GenericArray<unowned Gtk.ToggleButton> category_buttons;
}


class Gth.CategoryEntry : Object {
	public enum Type {
		ALL,
		MODIFIED,
		CATEGORY,
	}
	public Type type;
	public string title { get; set; }
	public ShortcutCategory category;

	public CategoryEntry.all (string _title) {
		type = Type.ALL;
		title = _title;
	}

	public CategoryEntry.modified (string _title) {
		type = Type.MODIFIED;
		title = _title;
	}

	public CategoryEntry.for_category (ShortcutCategory _category) {
		type = Type.CATEGORY;
		category = _category;
		title = _(category.to_title ());
	}

	public static int compare (CategoryEntry a, CategoryEntry b) {
		if (a.type == b.type) {
			if (a.type == CategoryEntry.Type.CATEGORY) {
				return Util.intcmp (a.category, b.category);
			}
			else {
				return 0;
			}
		}
		else {
			return Util.intcmp (a.type, b.type);
		}
	}
}
