public class Gth.GeneralFilterRow : Adw.ComboRow {
	construct {
		title = _("General Filter");

		var filters = new GenericList<Gth.Test>();
		foreach (unowned var id in GENERAL_FILTERS) {
			var filter = app.get_test_by_id (id);
			filters.model.append (filter.duplicate ());
		}

		var saved_filter_id = app.settings.get_string (PREF_BROWSER_GENERAL_FILTER);
		var general_filter_id = app.migration.test.get_new_key (saved_filter_id);
		var iter = filters.iterator ();
		var general_filter_pos = iter.find_first ((test) => test.id == general_filter_id);

		model = filters.model;
		expression = new Gtk.PropertyExpression (typeof (Gth.Test), null, "display-name");
		if (general_filter_pos >= 0) {
			selected = general_filter_pos;
		}

		notify["selected"].connect ((_row, _param) => {
			var test = model.get_item (selected) as Gth.Test;
			if (test != null) {
				var old_id = app.migration.test.get_old_key (test.id);
				app.settings.set_string (PREF_BROWSER_GENERAL_FILTER, old_id);
				activate_action_variant ("win.set-general-filter", new Variant.string (test.id));
			}
		});
	}

	const string[] GENERAL_FILTERS = {
		"Type::File",
		"Type::Media",
		"Type::Image",
		"Type::Video"
	};
}
