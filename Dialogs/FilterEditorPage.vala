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

		var sort_names = new Gtk.StringList (null);
		foreach (unowned var info in Sort_Name) {
			sort_names.append (info.display_name);
		}
		sort_name.model = sort_names;

		var unit_names = new Gtk.StringList (null);
		foreach (unowned var info in Test.Unit_List) {
			unit_names.append (info.display_name);
		}
		size_unit.model = unit_names;
	}

	void update_visibility () {
		match_operation_group.visible = (filter != null) && (filter.tests.tests.model.get_n_items () > 1);
	}

	public void set_filter (Gth.Filter? current_filter) {
		if (current_filter != null) {
			filter = current_filter.duplicate () as Gth.Filter;
		}
		else {
			filter = new Gth.Filter ();
			filter.visible = true;
		}

		filter.tests.tests.model.items_changed.connect (() => update_visibility ());
		update_visibility ();

		name_entry.set_text (filter.display_name);
		test_list.bind_model (filter.tests.tests.model, new_test_row);
		if (filter.tests.operation == Gth.TestChain.Operation.UNION) {
			match_any.active = true;
		}
		else {
			match_all.active = true;
		}
		if (filter.limit_type == Gth.Filter.LimitType.FILES) {
			limits_row.enable_expansion = true;
			limits_row.expanded = true;
			files_checkbox.active = true;
			files_limit.value = filter.limit;
		}
		else if (filter.limit_type == Gth.Filter.LimitType.BYTES) {
			limits_row.enable_expansion = true;
			limits_row.expanded = true;
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
			limits_row.enable_expansion = false;
			files_checkbox.active = true;
			files_limit.value = 1;
			size_limit.value = 1;
			size_unit.selected = 1;
		}
		sort_name.selected = SortInfo.index_of (Sort_Name, filter.sort_name);
		sort_type.selected = (filter.sort_type == Gtk.SortType.ASCENDING) ? 0 : 1;
	}

	public void add_rule (string id) {
		var registered_test = app.get_test_by_id (id);
		if (registered_test != null) {
			var new_test = registered_test.duplicate ();
			filter.tests.add (new_test);
			new_test.focus_options ();
		}
	}

	public Filter? get_filter () throws Error {
		// Name
		filter.display_name = name_entry.get_text ().strip ();
		if (Strings.empty (filter.display_name)) {
			name_entry.grab_focus ();
			throw new IOError.FAILED (_("Name is empty"));
		}

		// Tests
		foreach (unowned var test in filter.tests.tests) {
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
		filter.limit_type = !limits_row.enable_expansion ? Gth.Filter.LimitType.NONE : files_checkbox.active ? Gth.Filter.LimitType.FILES : Gth.Filter.LimitType.BYTES;
		if (filter.limit_type != Gth.Filter.LimitType.NONE) {
			if (filter.limit_type == Gth.Filter.LimitType.FILES) {
				filter.limit = (int64) files_limit.value;
			}
			else if (filter.limit_type == Gth.Filter.LimitType.BYTES) {
				filter.limit = ((int64) size_limit.value) * Test.Unit_List[size_unit.selected].size;
			}
			filter.sort_name = Sort_Name[sort_name.selected].id;
			filter.sort_type = Sort_Type[sort_type.selected];
		}
		else {
			filter.limit = 0;
			filter.sort_name = "";
		}

		return filter;
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
			filter.tests.remove (test);
		});
		row.add_suffix (delete_button);

		return row;
	}

	[GtkCallback]
	void on_save (Gtk.Button source) {
		save ();
	}

	[GtkCallback]
	void on_add_rule (Gtk.Button source) {
		choose_rule_type ();
	}

	struct SortInfo {
		string id;
		string display_name;

		public static int index_of (SortInfo[] list, string? id) {
			if (id != null) {
				var idx = 0;
				foreach (unowned var info in list) {
					if (info.id == id)
						return idx;
					idx++;
				}
			}
			return 0;
		}
	}

	const SortInfo[] Sort_Name = {
		{ "file::name", N_("Name") },
		{ "file::size", N_("Size") },
		{ "file::mtime", N_("Modified") },
	};

	const Gtk.SortType[] Sort_Type = {
		Gtk.SortType.ASCENDING,
		Gtk.SortType.DESCENDING,
	};

	[GtkChild] Adw.EntryRow name_entry;
	[GtkChild] Gtk.ListBox test_list;
	[GtkChild] Gtk.CheckButton match_all;
	[GtkChild] Gtk.CheckButton match_any;
	[GtkChild] Gtk.CheckButton files_checkbox;
	[GtkChild] Gtk.CheckButton size_checkbox;
	[GtkChild] Gtk.Adjustment files_limit;
	[GtkChild] Gtk.Adjustment size_limit;
	[GtkChild] Gtk.DropDown size_unit;
	[GtkChild] Adw.ComboRow sort_name;
	[GtkChild] Adw.ComboRow sort_type;
	[GtkChild] Adw.PreferencesGroup match_operation_group;
	[GtkChild] Adw.ExpanderRow limits_row;
}
