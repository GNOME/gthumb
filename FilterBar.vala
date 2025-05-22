public class Gth.FilterBar : Gtk.Box {
	public Gth.Test filter;

	public FilterBar () {
		filter = null;

		orientation = Gtk.Orientation.HORIZONTAL;
		spacing = HORIZONTAL_SPACING;
		margin_top = CONTAINER_V_PADDING;
		margin_bottom = CONTAINER_V_PADDING;
		margin_start = CONTAINER_H_PADDING;
		margin_end = CONTAINER_H_PADDING;

		visible_filters = app.get_visible_filters ();
		app.filter_file.changed.connect (() => {
			update_filter_list ();
		});

		// Label

		//var selector_label = new Gtk.Label.with_mnemonic (_("S_how:"));
		//selector_label.mnemonic_widget = filter_selector;

		// Filter selector

		var filter_actions = new Menu ();

		filter_section = new Menu ();
		update_filter_list ();
		// Translators: this is a verb. For example: Show Images.
		filter_actions.append_section (_("Show"), filter_section);

		var personalize_section = new Menu ();
		// Translators: the "Personalize filters" command.
		personalize_section.append (_("Personalize…"), "win.edit-filters");
		filter_actions.append_section (null, personalize_section);

		filter_selector = new Gtk.MenuButton ();
		filter_selector.always_show_arrow = true;
		filter_selector.menu_model = filter_actions;
		filter_label = new Gtk.Label ("");
		filter_selector.child = filter_label;

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
		var _filter = visible_filters.get_item (idx) as Gth.Test;
		if (_filter == null)
			return;

		filter = _filter;
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
		var iter = new ListModelIterator (visible_filters);
		while (iter.next ()) {
			unowned var filter = iter.get () as Gth.Test;
			if (filter.id == test_id) {
				test = filter;
				test_idx = iter.index ();
				return true;
			}
		}
		test = null;
		test_idx = -1;
		return false;
	}

	void update_filter_list () {
		var selected_id = (filter != null) ? filter.id : null;
		var selected_idx = -1;
		filter_section.remove_all ();
		visible_filters = app.get_visible_filters ();
		var iter = new ListModelIterator (visible_filters);
		while (iter.next ()) {
			unowned var _filter = iter.get () as Gth.Test;
			filter_section.append (_filter.display_name, "win.set-filter('%s')".printf (_filter.id));
			if (_filter.id == selected_id) {
				selected_idx = iter.index ();
			}
		}
		if (selected_idx == -1) {
			selected_id = "";
		}
		select_window_filter (selected_id);
	}

	Gtk.MenuButton filter_selector;
	Gtk.Box options_container;
	Gtk.Label filter_label;
	ListStore visible_filters;
	Menu filter_section;
}
