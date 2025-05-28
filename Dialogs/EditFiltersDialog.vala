[GtkTemplate (ui = "/app/gthumb/gthumb/ui/edit-filters-dialog.ui")]
public class Gth.EditFiltersDialog : Adw.PreferencesDialog {
	public EditFiltersDialog () {
		current_filter = null;

		var action_group = new SimpleActionGroup ();
		insert_action_group ("dialog", action_group);

		var action = new SimpleAction ("add-rule", GLib.VariantType.STRING);
		action.activate.connect ((_action, param) => {
			pop_subpage ();
			filter_page.add_rule (param.get_string ());
		});
		action_group.add_action (action);

		// General filter
		var general_filter_id = app.browser_settings.get_string (PREF_BROWSER_GENERAL_FILTER);
		var filters = app.get_file_type_filters ();
		var iter = filters.iterator ();
		var general_filter_pos = iter.find_first ((obj) => ((Gth.Test) obj).id == general_filter_id);

		general_filter_row.model = filters.model;
		general_filter_row.expression = new Gtk.PropertyExpression (typeof (Gth.Test), null, "display-name");
		if (general_filter_pos >= 0) {
			general_filter_row.selected = general_filter_pos;
		}

		// Filter list
		var filter_list = Widgets.new_boxed_list ();
		filter_list.bind_model (app.filter_file.filters.model, new_filter_row);
		filters_group.add (filter_list);
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

	bool rules_loaded = false;

	[GtkCallback]
	private void on_choose_rule_type (Gth.FilterEditorPage source) {
		if (!rules_loaded) {
			var test_list = Widgets.new_boxed_list ();
			test_list.bind_model (app.ordered_tests.model, new_test_type_row);
			rule_types.add (test_list);
			rules_loaded = true;
		}
		push_subpage (rule_page);
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
			app.browser_settings.set_string (PREF_BROWSER_GENERAL_FILTER, test.id);
			activate_action_variant ("set-general-filter", new Variant.string (test.id));
		}
	}

	Gtk.Widget new_test_type_row (Object item) {
		var test = item as Gth.Test;

		var row = new Adw.ActionRow ();
		row.title = test.display_name;
		row.activatable = true;
		row.action_name = "dialog.add-rule";
		row.action_target = test.id;

		var icon = new Gtk.Image.from_icon_name ("list-add-symbolic");
		row.add_suffix (icon);

		return row;
	}

	void move_row_to_position (Gth.FilterRow row, int target_pos) {
		var source_pos = row.get_index ();
		if ((target_pos >= 0) && (target_pos != source_pos)) {
			app.filter_file.filters.model.remove (source_pos);
			app.filter_file.filters.model.insert (target_pos, row.filter);
			app.filter_file.changed ();
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
			var last_pos = (int) app.filter_file.filters.model.get_n_items () - 1;
			move_row_to_position (source_row, last_pos);
		});

		if (filter is Gth.Filter) {
			row.edit_button.clicked.connect (() => {
				current_filter = filter as Gth.Filter;
				filter_page.set_filter (current_filter);
				push_subpage (filter_page);
			});
			row.delete_button.clicked.connect (() => {
				app.filter_file.remove (filter);
			});
		}

		row.visibility_switch.notify["active"].connect ((_obj, _prop) => {
			var local_switch = _obj as Gtk.Switch;
			filter.visible = local_switch.active;
			app.filter_file.changed ();
		});

		return row;
	}

	bool save_filter () {
		try {
			var filter = filter_page.get_filter ();
			if (current_filter == null) {
				app.filter_file.add (filter.duplicate ());
			}
			else {
				current_filter.copy (filter);
				app.filter_file.changed ();
			}
			return true;
		}
		catch (Error error) {
			add_toast (new Adw.Toast (error.message));
			return false;
		}
	}

	Gth.Filter current_filter;

	[GtkChild] private Adw.ComboRow general_filter_row;
	[GtkChild] private Adw.PreferencesGroup filters_group;
	[GtkChild] private Gth.FilterEditorPage filter_page;
	[GtkChild] private Adw.NavigationPage rule_page;
	[GtkChild] private Adw.PreferencesGroup rule_types;
}
