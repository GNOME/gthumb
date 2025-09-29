[GtkTemplate (ui = "/app/gthumb/gthumb/ui/filter-bar.ui")]
public class Gth.FilterBar : Gtk.Box {
	public Gth.Test filter;

	public signal void changed ();

	public void reset_filter () {
		if (select_filter_by_id ("")) {
			changed ();
		}
	}

	construct {
		// Actions

		action_group = new SimpleActionGroup ();
		insert_action_group ("filterbar", action_group);

		var action = new SimpleAction.stateful ("set-filter", GLib.VariantType.STRING, new Variant.string (""));
		action.activate.connect ((_action, param) => {
			var id = param.get_string ();
			if (select_filter_by_id (id)) {
				changed ();
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("reset-filter", null);
		action.activate.connect ((_action, param) => reset_filter ());
		action_group.add_action (action);

		// Menu

		var menu = new Menu ();

		filter_submenu = new Menu ();
		// Translators: this is a verb. For example: Show Images.
		menu.append_section (_("Show"), filter_submenu);

		var personalize = new Menu ();
		// Translators: the "Personalize filters" command.
		personalize.append (_("Personalize…"), "win.edit-filters");
		menu.append_section (null, personalize);

		filter_selector.menu_model = menu;

		filter = null;
		visible_filters = app.get_visible_filters ();
		app.filters.changed.connect ((changed_id) => {
			int current_filter_idx = -1;
			update_filter_list (out current_filter_idx);
			if (current_filter_idx == -1) {
				// The current filter is no longer visible.
				set_selected (0);
				changed ();
			}
			else {
				set_selected (current_filter_idx);
				if (changed_id == filter.id) {
					changed ();
				}
			}
		});
	}

	public bool select_filter_by_id (string id) {
		int test_idx;
		if (!find_filter_by_id (id, null, out test_idx))
			return false;
		return set_selected (test_idx);
	}

	public void general_filter_changed () {
		var state = action_group.get_action_state ("set-filter");
		if (state != null) {
			var general_filter_active = state.get_string () == "";
			update_filter_list (null);
			if (general_filter_active) {
				select_filter_by_id ("");
				changed ();
			}
		}
	}

	public void set_active_filter (Gth.Test? filter) {
		update_filter_list (null);
		Gth.Test test = null;
		int test_idx = 0;
		if ((filter != null) && find_filter_by_id (filter.id, out test, out test_idx)) {
			test.copy (filter);
			set_selected (test_idx);
		}
		else {
			// Select the general filter.
			select_filter_by_id ("");
		}
	}

	public void restore_from_file () {
		Gth.Test filter = null;
		var file = Gth.UserDir.get_config_file (Gth.FileIntent.READ, "active_filter.xml");
		if (file != null) {
			try {
				var doc = new Dom.Document ();
				doc.load_file (file);
				if (doc.first_child != null) {
					var id = doc.first_child.get_attribute ("id");
					if (!Strings.empty (id)) {
						var test = app.get_test_by_id (id);
						if (test != null) {
							filter = test.duplicate ();
						}
						else {
							filter = new Gth.Filter ();
						}
						filter.load_from_element (doc.first_child);
					}
				}
			}
			catch (Error error) {
				// Ignored.
			}
		}
		set_active_filter (filter);
	}

	public void save_to_file () {
		var file = Gth.UserDir.get_config_file (Gth.FileIntent.WRITE, "active_filter.xml");
		if (file == null)
			return;

		var content = "";
		if (filter != null) {
			var doc = new Dom.Document ();
			doc.append_child (filter.create_element (doc));
			content = doc.to_xml ();
		}
		try {
			Files.save_content (file, content, null);
		}
		catch (Error error) {
		}
	}

	ulong options_changed_id = 0;

	bool set_selected (int idx) {
		var _filter = visible_filters.model.get_item (idx) as Gth.Test;
		if (_filter == null) {
			return false;
		}

		if ((filter != null) && (options_changed_id != 0)) {
			filter.disconnect (options_changed_id);
			options_changed_id = 0;
		}

		filter = _filter;
		filter_label.set_text (filter.display_name);
		options_changed_id = filter.options_changed.connect ((obj) => {
			try {
				var test = obj as Test;
				test.update_from_options ();
				changed ();
			}
			catch (Error error) {
				// TODO: show error.
			}
		});

		var prev_options = options_container.get_first_child ();
		if (prev_options != null) {
			options_container.remove (prev_options);
		}

		var filter_options = filter.create_options ();
		if (filter_options != null) {
			options_container.append (filter_options);
			filter.focus_options ();
		}

		action_group.change_action_state ("set-filter", new Variant.string (filter.id));
		reset_filter_button.visible = (filter.id != "");

		return true;
	}

	bool find_filter_by_id (string test_id, out Test test, out int test_idx) {
		var iter = visible_filters.iterator ();
		while (iter.next ()) {
			unowned var filter = iter.get ();
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

	void update_filter_list (out int selected_idx) {
		selected_idx = -1;
		var selected_id = (filter != null) ? filter.id : null;
		filter_submenu.remove_all ();
		visible_filters = app.get_visible_filters ();
		var iter = visible_filters.iterator ();
		while (iter.next ()) {
			unowned var _filter = iter.get ();
			filter_submenu.append (_filter.display_name, "filterbar.set-filter('%s')".printf (_filter.id));
			if (_filter.id == selected_id) {
				selected_idx = iter.index ();
			}
		}
	}

	public void show_menu () {
		filter_selector.popup ();
	}

	[GtkChild] unowned Gtk.MenuButton filter_selector;
	[GtkChild] unowned Gtk.Box options_container;
	[GtkChild] unowned Gtk.Label filter_label;
	[GtkChild] unowned Gtk.Button reset_filter_button;
	Menu filter_submenu;
	GenericList<Gth.Test> visible_filters;
	SimpleActionGroup action_group;
}
