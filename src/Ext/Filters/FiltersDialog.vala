[GtkTemplate (ui = "/org/gnome/gthumb/ui/filters-dialog.ui")]
public class Gth.FiltersDialog : Adw.PreferencesDialog {
	public FiltersDialog () {
		current_filter = null;

		// General filter

		var filters = new GenericList<Gth.Test>();
		foreach (unowned var id in GENERAL_FILTERS) {
			var filter = app.get_test_by_id (id);
			filters.model.append (filter.duplicate ());
		}

		var saved_filter_id = app.settings.get_string (PREF_BROWSER_GENERAL_FILTER);
		var general_filter_id = app.migration.test.get_new_key (saved_filter_id);
		var iter = filters.iterator ();
		var general_filter_pos = iter.find_first ((test) => test.id == general_filter_id);

		general_filter_row.model = filters.model;
		general_filter_row.expression = new Gtk.PropertyExpression (typeof (Gth.Test), null, "display-name");
		if (general_filter_pos >= 0) {
			general_filter_row.selected = general_filter_pos;
		}

		// Filter list

		filter_list.bind_model (app.filters.entries.model, new_filter_row);

		var empty_row = new Adw.ActionRow ();
		empty_row.title = _("Empty");
		empty_row.sensitive = false;
		empty_row.halign = Gtk.Align.CENTER;
		filter_list.set_placeholder (empty_row);
	}

	[GtkCallback]
	private void on_cancel_filter (Gth.FilterEditorPage source) {
		pop_subpage ();
	}

	[GtkCallback]
	private void on_save_filter (Gth.FilterEditorPage source) {
		if (save_filter ()) {
			pop_subpage ();
		}
	}

	[GtkCallback]
	private void on_add_filter (Gtk.Button source) {
		current_filter = null;
		filter_page.set_filter (current_filter);
		push_subpage (filter_page);
	}

	[GtkCallback]
	private void on_general_filter_selected (Object row, ParamSpec param) {
		var test = general_filter_row.model.get_item (general_filter_row.selected) as Gth.Test;
		if (test != null) {
			var old_id = app.migration.test.get_old_key (test.id);
			app.settings.set_string (PREF_BROWSER_GENERAL_FILTER, old_id);
			activate_action_variant ("win.set-general-filter", new Variant.string (test.id));
		}
	}

	void move_row_to_position (Gth.FilterRow row, int target_pos) {
		var source_pos = row.get_index ();
		if ((target_pos >= 0) && (target_pos != source_pos)) {
			app.filters.entries.model.remove (source_pos);
			app.filters.entries.model.insert (target_pos, row.filter);
			app.filters.changed (null);
		}
	}

	Gtk.Widget new_filter_row (Object item) {
		var filter = item as Gth.Test;

		var row = new Gth.FilterRow (filter);
		row.move_to_row.connect ((source_row, target_row) => {
			move_row_to_position (source_row, target_row.get_index ());
		});
		row.move_to_top.connect ((source_row) => {
			move_row_to_position (source_row, 0);
		});
		row.move_to_bottom.connect ((source_row) => {
			var last_pos = (int) app.filters.entries.model.get_n_items () - 1;
			move_row_to_position (source_row, last_pos);
		});
		row.delete_row.connect ((source_row) => {
			app.filters.remove (filter);
		});

		if (filter is Gth.Filter) {
			row.edit_button.clicked.connect (() => {
				current_filter = filter as Gth.Filter;
				filter_page.set_filter (current_filter);
				push_subpage (filter_page);
			});
		}

		row.visibility_switch.notify["active"].connect ((_obj, _prop) => {
			var local_switch = _obj as Gtk.Switch;
			filter.visible = local_switch.active;
			app.filters.changed (filter.id);
		});

		return row;
	}

	bool save_filter () {
		try {
			var filter = filter_page.get_filter ();
			if (current_filter == null) {
				app.filters.add (filter.duplicate ());
			}
			else {
				current_filter.copy (filter);
				app.filters.changed (filter.id);
			}
			return true;
		}
		catch (Error error) {
			add_toast (Util.new_literal_toast (error.message));
			return false;
		}
	}

	Gth.Filter current_filter;

	[GtkChild] unowned Adw.ComboRow general_filter_row;
	[GtkChild] unowned Gtk.ListBox filter_list;
	[GtkChild] unowned Gth.FilterEditorPage filter_page;

	const string[] GENERAL_FILTERS = {
		"Type::File",
		"Type::Media",
		"Type::Image",
		"Type::Video"
	};
}
