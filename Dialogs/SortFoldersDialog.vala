[GtkTemplate (ui = "/app/gthumb/gthumb/ui/sort-folders-dialog.ui")]
public class Gth.SortFoldersDialog : Adw.PreferencesDialog {
	public SortFoldersDialog (Gth.Browser _browser) {
		browser = _browser;

		sort_name = browser.folder_tree.sort.name;
		inverse_order = browser.folder_tree.sort.inverse;

		Gtk.CheckButton first_check_button = null;
		var idx = 0;
		foreach (unowned var id in FOLDER_SORTERS) {
			unowned var info = app.sorters.get (id);
			var row = new Adw.ActionRow ();
			var sort_id = SORTER_ID[idx];
			row.title = info.display_name;
			var check_button = new Gtk.CheckButton ();
			if (sort_id == sort_name) {
				check_button.active = true;
			}
			if (first_check_button == null) {
				first_check_button = check_button;
			}
			else {
				check_button.group = first_check_button;
			}
			check_button.toggled.connect (() => {
				sort_name = sort_id;
				browser.folder_tree.set_sort_order (sort_name, inverse_order);
			});
			row.add_prefix (check_button);
			row.activatable_widget = check_button;

			rule_types.add (row);
			idx++;
		}

		inverse_order_switch.active = inverse_order;
		inverse_order_switch.notify["active"].connect (() => {
			inverse_order = inverse_order_switch.active;
			browser.folder_tree.set_sort_order (sort_name, inverse_order);
		});
	}

	weak Gth.Browser browser;
	string sort_name;
	bool inverse_order;

	[GtkChild] unowned Adw.PreferencesGroup rule_types;
	[GtkChild] unowned Adw.SwitchRow inverse_order_switch;

	const string[] FOLDER_SORTERS = {
		"File::Name",
		"Time::Modified"
	};

	const string[] SORTER_ID = {
		"name",
		"modification-time",
	};
}
