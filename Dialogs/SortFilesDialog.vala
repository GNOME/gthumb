[GtkTemplate (ui = "/app/gthumb/gthumb/ui/sort-files-dialog.ui")]
public class Gth.SortFilesDialog : Adw.ApplicationWindow {
	public SortFilesDialog (Gth.Window _window) {
		window = _window;
		transient_for = window;

		if (window.folder != null) {
			sort_type = window.folder.info.get_attribute_string ("sort::type");
			inverse_order = window.folder.info.get_attribute_boolean ("sort::inverse");
		}
		else {
			sort_type = window.sort_type;
			inverse_order = window.inverse_order;
		}

		Gtk.CheckButton first_check_button = null;
		foreach (unowned var id in app.ordered_sorters) {
			unowned var info = app.sorters.get (id);
			var row = new Adw.ActionRow ();
			row.title = info.display_name;
			var check_button = new Gtk.CheckButton ();
			if (id == sort_type) {
				check_button.active = true;
			}
			check_button.toggled.connect (() => set_sort_type (id));
			if (first_check_button == null) {
				first_check_button = check_button;
			}
			else {
				check_button.group = first_check_button;
			}
			row.add_prefix (check_button);
			row.activatable_widget = check_button;

			rule_types.add (row);
		}

		inverse_order_switch.active = inverse_order;
	}

	void set_sort_type (string id) {
		sort_type = id;
		window.update_sort_order (sort_type, inverse_order);
	}

	void activate_inverse_order (bool active) {
		inverse_order = active;
		window.update_sort_order (sort_type, inverse_order);
	}

	weak Gth.Window window;
	string sort_type;
	bool inverse_order;

	[GtkChild] Adw.PreferencesGroup rule_types;
	[GtkChild] Adw.SwitchRow inverse_order_switch;
}
