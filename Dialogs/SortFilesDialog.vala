[GtkTemplate (ui = "/app/gthumb/gthumb/ui/sort-files-dialog.ui")]
public class Gth.SortFilesDialog : Adw.ApplicationWindow {
	public SortFilesDialog (Gth.Window _window) {
		window = _window;
		transient_for = window;

		sort = {
			window.folder_tree.current_folder.get_sort_name (window.sort.name),
			window.folder_tree.current_folder.get_inverse_order (window.sort.inverse)
		};

		Gtk.CheckButton first_check_button = null;
		foreach (unowned var id in app.ordered_sorters) {
			unowned var info = app.sorters.get (id);
			var row = new Adw.ActionRow ();
			row.title = info.display_name;
			var check_button = new Gtk.CheckButton ();
			if (id == sort.name) {
				check_button.active = true;
			}
			if (first_check_button == null) {
				first_check_button = check_button;
			}
			else {
				check_button.group = first_check_button;
			}
			check_button.toggled.connect (() => {
				sort.name = id;
				window.update_sort_order (sort.name, sort.inverse);
			});
			row.add_prefix (check_button);
			row.activatable_widget = check_button;

			rule_types.add (row);
		}

		inverse_order_switch.active = sort.inverse;
		inverse_order_switch.notify["active"].connect (() => {
			sort.inverse = inverse_order_switch.active;
			window.update_sort_order (sort.name, sort.inverse);
		});
	}

	weak Gth.Window window;
	Gth.Sort sort;

	[GtkChild] unowned Adw.PreferencesGroup rule_types;
	[GtkChild] unowned Adw.SwitchRow inverse_order_switch;
}
