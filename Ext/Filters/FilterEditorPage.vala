[GtkTemplate (ui = "/app/gthumb/gthumb/ui/filter-editor-page.ui")]
public class Gth.FilterEditorPage : Adw.NavigationPage {
	public Gth.Filter filter;

	public signal void cancelled ();
	public signal void save ();

	construct {
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

	public void set_filter (Gth.Filter? current_filter) {
		if (current_filter != null) {
			filter = current_filter.duplicate () as Gth.Filter;
		}
		else {
			filter = new Gth.Filter ();
			filter.visible = true;
		}

		name_entry.set_text (filter.display_name);

		rules_group.set_expr (filter.tests);

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
		sort_name.selected = SortInfo.index_of (Sort_Name, filter.sort.name);
		sort_type.selected = filter.sort.inverse ? 1 : 0;
	}

	public Filter? get_filter () throws Error {
		// Name
		filter.display_name = name_entry.get_text ().strip ();
		if (Strings.empty (filter.display_name)) {
			name_entry.grab_focus ();
			throw new IOError.FAILED (_("Name is empty"));
		}

		// Rules
		rules_group.update_from_options ();

		// Limits
		filter.limit_type = !limits_row.enable_expansion ? Gth.Filter.LimitType.NONE : files_checkbox.active ? Gth.Filter.LimitType.FILES : Gth.Filter.LimitType.BYTES;
		if (filter.limit_type != Gth.Filter.LimitType.NONE) {
			if (filter.limit_type == Gth.Filter.LimitType.FILES) {
				filter.limit = (int64) files_limit.value;
			}
			else if (filter.limit_type == Gth.Filter.LimitType.BYTES) {
				filter.limit = ((int64) size_limit.value) * Test.Unit_List[size_unit.selected].size;
			}
			filter.sort = {
				Sort_Name[sort_name.selected].id,
				Sort_Type[sort_type.selected] == Gtk.SortType.DESCENDING
			};
		}
		else {
			filter.limit = 0;
			filter.sort = { null, false };
		}

		return filter;
	}

	[GtkCallback]
	void on_save (Gtk.Button source) {
		save ();
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
		{ "File::Name", N_("Name") },
		{ "File::Size", N_("Size") },
		{ "Time::Modified", N_("Modified") },
	};

	const Gtk.SortType[] Sort_Type = {
		Gtk.SortType.ASCENDING,
		Gtk.SortType.DESCENDING,
	};

	[GtkChild] unowned Adw.EntryRow name_entry;
	[GtkChild] unowned Gth.TestExprEditorGroup rules_group;
	[GtkChild] unowned Gtk.CheckButton files_checkbox;
	[GtkChild] unowned Gtk.CheckButton size_checkbox;
	[GtkChild] unowned Gtk.Adjustment files_limit;
	[GtkChild] unowned Gtk.Adjustment size_limit;
	[GtkChild] unowned Gtk.DropDown size_unit;
	[GtkChild] unowned Adw.ComboRow sort_name;
	[GtkChild] unowned Adw.ComboRow sort_type;
	[GtkChild] unowned Adw.ExpanderRow limits_row;
}
