[GtkTemplate (ui = "/app/gthumb/gthumb/ui/filter-editor-page.ui")]
public class Gth.FilterEditorPage : Adw.NavigationPage {
	public Gth.Filter filter;

	public signal void cancelled ();
	public signal void save ();
	public signal void choose_rule_type ();

	construct {
		var empty_row = new Adw.ActionRow ();
		empty_row.title = _("No Rule");
		empty_row.sensitive = false;
		empty_row.halign = Gtk.Align.CENTER;
		test_list.set_placeholder (empty_row);
	}

	public void set_filter (Gth.Filter? current_filter) {
		if (current_filter != null) {
			filter = current_filter.duplicate () as Gth.Filter;
		}
		else {
			filter = new Gth.Filter ();
		}
		name_entry.set_text (filter.display_name);
		test_list.bind_model (filter.tests.tests, new_test_row);
		if (filter.tests.operation == Gth.TestChain.Operation.INTERSECTION)
			match_all.active = true;
		else
			match_any.active = true;
		if (filter.limit_type == Gth.Filter.LimitType.FILES) {
			files_checkbox.active = true;
			files_limit.value = filter.limit;
		}
		else if (filter.limit_type == Gth.Filter.LimitType.BYTES) {
			size_checkbox.active = true;

			var idx = 0;
			foreach (unowned var info in Test.Unit_List) {
				if ((idx == Test.Unit_List.length - 1)
					|| (filter.limit < Test.Unit_List[idx + 1].size))
				{
					size_unit.selected = idx;
					size_limit.value = (double) filter.limit / Test.Unit_List[idx].size;
					break;
				}
				idx++;
			}
		}
		else {
			no_limit_checkbox.active = true;
			files_limit.value = 1;
			size_limit.value = 1;
			size_unit.selected = 1;
		}
		sort_name.selected = Util.enum_index (Sort_Name, filter.sort_name);
		sort_type.selected = (filter.sort_type == Gtk.SortType.ASCENDING) ? 0 : 1;
	}

	public void add_rule (string id) {
		var registered_test = app.get_test_by_id (id);
		if (registered_test != null) {
			var new_test = registered_test.duplicate ();
			filter.tests.add_test (new_test);
			new_test.focus_options ();
		}
	}

	public Filter? get_filter () throws Error {
		// Name
		filter.display_name = name_entry.get_text ();
		if (Strings.empty (filter.display_name)) {
			name_entry.grab_focus ();
			throw new IOError.FAILED (_("Name is empty"));
		}

		// Tests
		var iter = new ListModelIterator (filter.tests.tests);
		while (iter.next ()) {
			unowned var test = iter.get () as Test;
			try {
				test.update_from_options ();
			}
			catch (Error error) {
				test.focus_options ();
				throw error;
			}
		}

		// Match All / Match Any
		filter.tests.operation = match_all.active ? Gth.TestChain.Operation.INTERSECTION : Gth.TestChain.Operation.UNION;
		filter.limit_type = files_checkbox.active ? Gth.Filter.LimitType.FILES : size_checkbox.active ? Gth.Filter.LimitType.BYTES : Gth.Filter.LimitType.NONE;
		if (filter.limit_type == Gth.Filter.LimitType.FILES) {
			filter.limit = (int64) files_limit.value;
		}
		else if (filter.limit_type == Gth.Filter.LimitType.BYTES) {
			filter.limit = ((int64) size_limit.value) * Unit_Size[size_unit.selected];
		}
		else {
			filter.limit = 0;
		}
		filter.sort_name = Sort_Name[sort_name.selected];
		filter.sort_type = Sort_Type[sort_type.selected];

		return null;
	}

	Gtk.Widget new_test_row (Object item) {
		var test = item as Gth.Test;

		var row = new Adw.ActionRow ();
		row.title = test.display_name;

		var icon = new Gtk.Image.from_icon_name ("filter-symbolic");
		row.add_prefix (icon);

		var test_options = test.create_options ();
		if (test_options != null) {
			test_options.valign = Gtk.Align.CENTER;
			row.add_suffix (test_options);
		}

		var delete_button = new Gtk.Button.from_icon_name ("list-delete-symbolic");
		delete_button.add_css_class ("flat");
		delete_button.valign = Gtk.Align.CENTER;
		delete_button.clicked.connect (() => {
			filter.tests.remove_test (test);
		});
		row.add_suffix (delete_button);

		return row;
	}

	[GtkCallback]
	void on_save (Gtk.Button source) {
		save ();
	}

	[GtkCallback]
	void on_add_rule (Adw.ButtonRow source) {
		choose_rule_type ();
	}

	const int64[] Unit_Size = {
		1024,
		1024 * 1024,
		1024 * 1024 * 1024
	};

	const string[] Sort_Name = {
		"file::name",
		"file::size",
		"file::mtime",
	};

	const Gtk.SortType[] Sort_Type = {
		Gtk.SortType.ASCENDING,
		Gtk.SortType.DESCENDING,
	};

	[GtkChild] Adw.EntryRow name_entry;
	[GtkChild] Gtk.ListBox test_list;
	[GtkChild] Gtk.CheckButton match_all;
	[GtkChild] Gtk.CheckButton match_any;
	[GtkChild] Gtk.CheckButton no_limit_checkbox;
	[GtkChild] Gtk.CheckButton files_checkbox;
	[GtkChild] Gtk.CheckButton size_checkbox;
	[GtkChild] Gtk.Adjustment files_limit;
	[GtkChild] Gtk.Adjustment size_limit;
	[GtkChild] Gtk.DropDown size_unit;
	[GtkChild] Adw.ComboRow sort_name;
	[GtkChild] Adw.ComboRow sort_type;
}
