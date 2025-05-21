[GtkTemplate (ui = "/app/gthumb/gthumb/ui/personalize-filters.ui")]
public class Gth.PersonalizeFilters : Adw.PreferencesDialog {
	public PersonalizeFilters () {
		settings = new GLib.Settings (GTHUMB_BROWSER_SCHEMA);

		// General filter
		var filters = app.get_file_type_filters ();
		var general_filter = settings.get_string (PREF_BROWSER_GENERAL_FILTER);
		var iter = new ListModelIterator (filters);
		var selected_filter = iter.find_first ((obj) => ((Gth.Test) obj).id == general_filter);

		general_filter_row.model = filters;
		general_filter_row.expression = new Gtk.PropertyExpression (typeof (Gth.Test), null, "display-name");
		if (selected_filter >= 0) {
			general_filter_row.selected = selected_filter;
		}

		// Filter list
		var filter_list = Widgets.new_list_box ();
		filter_list.bind_model (app.filter_file.filters, new_filter_row);
		filters_group.add (filter_list);

		// Filter page
		filter_page = new Gth.FilterEditorPage ();
		filter_page.cancelled.connect (() => pop_subpage ());
		filter_page.save.connect (() => {
			pop_subpage ();
		});
	}

	[GtkCallback]
	private void on_add_filter (Gtk.Button source) {
		filter_page.set_filter (null);
		push_subpage (filter_page);
	}

	[GtkCallback]
	private void on_general_filter_selected (Object row, ParamSpec param) {
		var test = general_filter_row.model.get_item (general_filter_row.selected) as Gth.Test;
		if (test != null) {
			settings.set_string (PREF_BROWSER_GENERAL_FILTER, test.id);
		}
	}

	Gtk.Widget new_filter_row (Object item) {
		var filter = item as Gth.Test;

		var row = new Adw.ActionRow ();
		row.title = filter.display_name;

		var drag_icon = new Gtk.Image.from_icon_name ("list-drag-handle-symbolic");
		row.add_prefix (drag_icon);

		if (filter is Gth.Filter) {
			var buttons = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 12);
			buttons.margin_end = 12;
			row.add_suffix (buttons);

			var edit_button = new Gtk.Button.from_icon_name ("edit-item-symbolic");
			edit_button.valign = Gtk.Align.CENTER;
			buttons.append (edit_button);

			var delete_button = new Gtk.Button.from_icon_name ("delete-item-symbolic");
			delete_button.add_css_class ("destructive-action");
			delete_button.valign = Gtk.Align.CENTER;
			buttons.append (delete_button);
		}

		var visibility_switch = new Gtk.Switch ();
		visibility_switch.valign = Gtk.Align.CENTER;
		visibility_switch.active = filter.visible;
		row.add_suffix (visibility_switch);
		row.activatable_widget = visibility_switch;

		return row;
	}

	GLib.Settings settings;
	Gth.FilterEditorPage filter_page;
	[GtkChild] private Adw.ComboRow general_filter_row;
	[GtkChild] private Adw.PreferencesGroup filters_group;
}
