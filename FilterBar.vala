public class Gth.FilterBar : Gtk.Box {
	public FilterBar () {
		orientation = Gtk.Orientation.HORIZONTAL;
		spacing = HORIZONTAL_SPACING;
		margin_top = CONTAINER_V_PADDING;
		margin_bottom = CONTAINER_V_PADDING;
		margin_start = CONTAINER_H_PADDING;
		margin_end = CONTAINER_H_PADDING;

		visible_filters = new GenericArray<Gth.Test>();
		var no_filter = new Gth.Test ();
		no_filter.id = "";
		no_filter.display_name = _("All Files");
		visible_filters.add (no_filter);
		foreach (unowned var filter in app.filter_file.filters) {
			if (filter.visible) {
				visible_filters.add (filter.duplicate ());
			}
		}

		// Label

		//var selector_label = new Gtk.Label.with_mnemonic (_("S_how:"));
		//selector_label.mnemonic_widget = filter_selector;

		// Filter selector

		var filter_actions = new Menu ();

		var filter_section = new Menu ();
		foreach (unowned var filter in visible_filters) {
			filter_section.append (filter.display_name, "win.set-filter('%s')".printf (filter.id));
		}
		// Translators: this is a verb. For example: Show images.
		filter_actions.append_section (_("Show"), filter_section);

		filter_section = new Menu ();
		// Translators: the "Personalize filters" command.
		filter_section.append (_("Personalize…"), "win.edit-filters");
		filter_actions.append_section (null, filter_section);

		filter_selector = new Gtk.MenuButton ();
		filter_label = new Gtk.Label ("");
		filter_selector.child = filter_label;
		filter_selector.set_menu_model (filter_actions);

		// Test options container.

		options_container = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);

		//append (selector_label);
		append (filter_selector);
		append (options_container);
	}

	public void select_filter_by_id (string id) {
		int test_idx;
		if (find_filter_by_id (id, null, out test_idx)) {
			set_selected (test_idx);
		}
	}

	public void load_active_filter () {
		if (!load_active_filter_from_file ()) {
			select_window_filter ("");
		}
	}

	void select_window_filter (string id) {
		activate_action_variant ("win.set-filter", new Variant.string (id));
	}

	public bool load_active_filter_from_file (string filename = "active_filter.xml") {
		var file = Gth.UserDir.get_app_file (Gth.UserDirType.CONFIG, Gth.FileIntent.WRITE, filename);
		if (file != null) {
			var doc = new Dom.Document ();
			try {
				doc.load_file (file);
			}
			catch (Error error) {
				return false;
			}
			if (doc.first_child != null) {
				var id = doc.first_child.get_attribute ("id");
				if (id != null) {
					Test test;
					int test_idx;
					if (find_filter_by_id (id, out test, out test_idx)) {
						test.load_from_element (doc.first_child);
						select_window_filter (id);
						return true;
					}
				}
			}
		}
		return false;
	}

	void set_selected (int idx) {
		var filter = visible_filters[idx];
		if (filter == null)
			return;

		filter_label.set_text (filter.display_name);

		var prev_options = options_container.get_first_child ();
		if (prev_options != null) {
			options_container.remove (prev_options);
		}

		var filter_options = filter.create_options ();
		if (filter_options != null) {
			options_container.append (filter_options);
			filter.focus_options ();
		}
	}

	bool find_filter_by_id (string test_id, out Test test, out int test_idx) {
		for (var idx = 0; idx < visible_filters.length; idx++) {
			var filter = visible_filters[idx];
			if (filter.id == test_id) {
				test = filter;
				test_idx = idx;
				return true;
			}
		}
		test = null;
		test_idx = -1;
		return false;
	}

	Gtk.MenuButton filter_selector;
	Gtk.Box options_container;
	Gtk.Label filter_label;
	GenericArray<Gth.Test> visible_filters;
}
