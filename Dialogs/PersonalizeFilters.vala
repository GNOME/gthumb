[GtkTemplate (ui = "/app/gthumb/gthumb/ui/personalize-filters.ui")]
public class Gth.PersonalizeFilters : Adw.PreferencesDialog {
	public PersonalizeFilters () {
		current_filter = null;

		var action_group = new SimpleActionGroup ();
		insert_action_group ("dialog", action_group);

		var action = new SimpleAction ("add-rule", GLib.VariantType.STRING);
		action.activate.connect ((param) => {
			pop_subpage ();
			filter_page.add_rule (param.get_string ());
		});
		action_group.add_action (action);

		// General filter
		var general_filter_id = app.browser_settings.get_string (PREF_BROWSER_GENERAL_FILTER);
		var filters = app.get_file_type_filters ();
		var iter = new ListModelIterator (filters);
		var general_filter_pos = iter.find_first ((obj) => ((Gth.Test) obj).id == general_filter_id);

		general_filter_row.model = filters;
		general_filter_row.expression = new Gtk.PropertyExpression (typeof (Gth.Test), null, "display-name");
		if (general_filter_pos >= 0) {
			general_filter_row.selected = general_filter_pos;
		}

		// Filter list
		var filter_list = Widgets.new_list_box ();
		filter_list.bind_model (app.filter_file.filters, new_filter_row);
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
			var test_list = Widgets.new_list_box ();
			test_list.bind_model (app.ordered_tests, new_test_type_row);
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
			app.filter_file.changed ();
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

	Gtk.Widget new_filter_row (Object item) {
		var filter = item as Gth.Test;

		var row = new Adw.ActionRow ();
		row.title = filter.display_name;
		filter.bind_property ("display_name", row, "title", BindingFlags.DEFAULT);

		var drag_icon = new Gtk.Image.from_icon_name ("list-drag-handle-symbolic");
		drag_icon.opacity = 0.5;
		row.add_prefix (drag_icon);

		if (filter is Gth.Filter) {
			var buttons = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 12);
			buttons.margin_end = 12;
			row.add_suffix (buttons);

			var edit_button = new Gtk.Button.from_icon_name ("edit-item-symbolic");
			edit_button.valign = Gtk.Align.CENTER;
			edit_button.clicked.connect (() => {
				current_filter = filter as Gth.Filter;
				filter_page.set_filter (current_filter);
				push_subpage (filter_page);
			});
			buttons.append (edit_button);

			var delete_button = new Gtk.Button.from_icon_name ("delete-item-symbolic");
			delete_button.add_css_class ("destructive-action");
			delete_button.valign = Gtk.Align.CENTER;
			delete_button.clicked.connect (() => {
				app.filter_file.remove (filter);
			});
			buttons.append (delete_button);
		}

		var visibility_switch = new Gtk.Switch ();
		visibility_switch.valign = Gtk.Align.CENTER;
		visibility_switch.active = filter.visible;
		row.add_suffix (visibility_switch);
		row.activatable_widget = visibility_switch;
		visibility_switch.notify["active"].connect ((_obj, _prop) => {
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
