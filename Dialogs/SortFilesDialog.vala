[GtkTemplate (ui = "/app/gthumb/gthumb/ui/sort-files-dialog.ui")]
public class Gth.SortFilesDialog : Adw.ApplicationWindow {
	public SortFilesDialog (Gth.Window _window) {
		window = _window;
		transient_for = window;

		if (window.folder_tree.current_folder != null) {
			sort_name = window.folder_tree.current_folder.info.get_attribute_string ("sort::name");
			inverse_order = window.folder_tree.current_folder.info.get_attribute_boolean ("sort::inverse");
		}

		if (sort_name == null) {
			sort_name = window.sort_name;
			inverse_order = window.inverse_order;
		}

		Gtk.CheckButton first_check_button = null;
		foreach (unowned var id in app.ordered_sorters) {
			unowned var info = app.sorters.get (id);
			var row = new Adw.ActionRow ();
			row.title = info.display_name;
			var check_button = new Gtk.CheckButton ();
			if (id == sort_name) {
				check_button.active = true;
			}
			if (first_check_button == null) {
				first_check_button = check_button;
			}
			else {
				check_button.group = first_check_button;
			}
			check_button.toggled.connect (() => {
				sort_name = id;
				window.update_sort_order (sort_name, inverse_order);
			});
			row.add_prefix (check_button);
			row.activatable_widget = check_button;

			rule_types.add (row);
		}

		inverse_order_switch.active = inverse_order;
		inverse_order_switch.notify["active"].connect (() => {
			inverse_order = inverse_order_switch.active;
			window.update_sort_order (sort_name, inverse_order);
		});
	}

	weak Gth.Window window;
	string sort_name;
	bool inverse_order;

	[GtkChild] unowned Adw.PreferencesGroup rule_types;
	[GtkChild] unowned Adw.SwitchRow inverse_order_switch;
}
