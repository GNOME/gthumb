[GtkTemplate (ui = "/org/gnome/gthumb/ui/sort-files-dialog.ui")]
public class Gth.SortFilesDialog : Adw.PreferencesDialog {
	public SortFilesDialog (Gth.Browser _browser) {
		browser = _browser;

		sort = { browser.file_sorter.name, browser.file_sorter.inverse };

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
				browser.set_file_order (sort.name, sort.inverse);
			});
			row.add_prefix (check_button);
			row.activatable_widget = check_button;

			rule_types.add (row);
		}

		inverse_order_switch.active = sort.inverse;
		inverse_order_switch.notify["active"].connect (() => {
			sort.inverse = inverse_order_switch.active;
			browser.set_file_inverse_order (sort.inverse);
		});
	}

	weak Gth.Browser browser;
	Gth.Sort sort;

	[GtkChild] unowned Adw.PreferencesGroup rule_types;
	[GtkChild] unowned Adw.SwitchRow inverse_order_switch;
}
