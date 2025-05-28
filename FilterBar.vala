[GtkTemplate (ui = "/app/gthumb/gthumb/ui/filter-bar.ui")]
public class Gth.FilterBar : Gtk.Box {
	public Gth.Test filter;

	public signal void changed ();

	construct {
		// Actions

		var action_group = new SimpleActionGroup ();
		insert_action_group ("filterbar", action_group);

		var action = new SimpleAction.stateful ("set-filter", GLib.VariantType.STRING, new Variant.string (""));
		action.activate.connect ((_action, param) => {
			select_filter_by_id (param.get_string ());
		});
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
		app.filter_file.changed.connect (() => {
			update_filter_list ();
		});
	}

	public void select_filter_by_id (string id) {
		int test_idx;
		if (find_filter_by_id (id, null, out test_idx)) {
			set_selected (test_idx);
		}
	}

	public void set_active_filter (Gth.Test? filter) {
		update_filter_list ();
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

	void set_selected (int idx) {
		var _filter = visible_filters.model.get_item (idx) as Gth.Test;
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

	void update_filter_list () {
		var selected_id = (filter != null) ? filter.id : null;
		var selected_idx = -1;
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
		if (selected_idx == -1) {
			selected_idx = 0;
		}
		set_selected (selected_idx);
	}

	[GtkChild] Gtk.MenuButton filter_selector;
	[GtkChild] Gtk.Box options_container;
	[GtkChild] Gtk.Label filter_label;
	Menu filter_submenu;
	GenericList<Gth.Test> visible_filters;
}
