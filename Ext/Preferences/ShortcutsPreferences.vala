[GtkTemplate (ui = "/app/gthumb/gthumb/ui/shortcuts-preferences.ui")]
public class Gth.ShortcutsPreferences : Adw.NavigationPage {
	GenericList<CategoryEntry> categories;

	construct {
		search_bar.connect_entry (search);
		sorted_shortcuts = app.shortcuts.entries.duplicate ();
		sorted_shortcuts.sort ((a, b) => Util.intcmp (a.category, b.category));

		categories = new GenericList<CategoryEntry> ();
		categories.model.append (new CategoryEntry.all (_("All Shortcuts")));
		categories.model.append (new CategoryEntry.modified (_("Modified Shortcuts")));
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

		category_selector.expression = new Gtk.PropertyExpression (typeof (CategoryEntry), null, "title");
		category_selector.model = categories.model;
		category_selector.notify["selected"].connect (() => update_search ());
		category_selector.selected = 0;
		update_list (CategoryEntry.Type.ALL);
	}

	GenericList<Shortcut> sorted_shortcuts;
	GenericArray<weak Adw.PreferencesGroup> groups = null;

	void update_list (CategoryEntry.Type filter_type, ShortcutCategory category = ShortcutCategory.HIDDEN) {
		if (groups != null) {
			while (groups.length > 0) {
				shortcut_page.remove (groups[0]);
				groups.remove_index (0);
			}
		}
		if (!search_bar.search_mode_enabled) {
			filter_type = CategoryEntry.Type.ALL;
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
			var categories = new GenericList<CategoryEntry> ();
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
		var entry = category_selector.model.get_item (category_selector.selected) as CategoryEntry;
		update_list (entry.type, entry.category);
	}

	[GtkChild] unowned Adw.PreferencesPage shortcut_page;
	[GtkChild] unowned Gtk.DropDown category_selector;
	[GtkChild] unowned Gtk.SearchEntry search;
	[GtkChild] unowned Gtk.SearchBar search_bar;
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
}
