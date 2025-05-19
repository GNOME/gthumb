public class Gth.Window : Gtk.ApplicationWindow {
	public enum Page {
		NONE = 0,
		BROWSER,
		VIEWER,
	}

	SimpleActionGroup action_group;

	void init_actions () {
		action_group = new SimpleActionGroup ();
		insert_action_group ("win", action_group);

		var action = new SimpleAction ("edit-filters", null);
		action.activate.connect (() => {
			// TODO
			stdout.printf ("=> edit-filters\n");
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("set-filter", GLib.VariantType.STRING, new Variant.string (""));
		action.activate.connect ((_action, param) => {
			_action.set_state (param);
			filter_bar.select_filter_by_id (param.get_string ());
		});
		action_group.add_action (action);
	}

	public Window (Gtk.Application _app, File location, File? file_to_select) {
		Object (application: app);
		browser_settings = new GLib.Settings (GTHUMB_BROWSER_SCHEMA);
		init_actions ();

		// Headerbar

		header_bar = new Gtk.HeaderBar ();
		set_titlebar (header_bar);

		// Content

		main_stack = new Gtk.Stack ();
		main_stack.transition_type = Gtk.StackTransitionType.CROSSFADE;
		child = main_stack;

		browser_page = build_browser_page ();
		main_stack.add_child (browser_page);

		viewer_page = new Gtk.Label ("VIEWER");
		main_stack.add_child (viewer_page);

		set_page (Page.BROWSER);

		// Restore the window size.

		var width = browser_settings.get_int (PREF_BROWSER_WINDOW_WIDTH);
		var height = browser_settings.get_int (PREF_BROWSER_WINDOW_HEIGHT);
		set_default_size (width, height);

		if (browser_settings.get_boolean (PREF_BROWSER_WINDOW_MAXIMIZED)) {
			maximize ();
		}

		// Load the location.

		Util.next_tick (() => {
			filter_bar.load_active_filter ();
			if (file_to_select != null)
				go_to (location, file_to_select);
			else
				load_location (location);
		});
	}

	public void go_to (File location, File file_to_select) {
		stdout.printf ("go to '%s' (select: '%s')\n", location.get_uri (), file_to_select.get_uri ());
	}

	public void load_location (File location) {
		stdout.printf ("load %s\n", location.get_uri ());
	}

	public void set_page (Page page) {
		if (page == current_page)
			return;
		current_page = page;
		switch (current_page) {
		case Page.BROWSER:
			main_stack.set_visible_child (browser_page);
			break;
		case Page.VIEWER:
			main_stack.set_visible_child (viewer_page);
			break;
		}
		update_title ();
		update_sensitivity ();
	}

	void update_title () {
		// TODO
		title = "gThumb";
	}

	void update_sensitivity () {
		// TODO
	}

	Gtk.Widget build_browser_page () {
		var box = new Gtk.Box (Gtk.Orientation.VERTICAL, 0);

		var expander = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
		expander.vexpand = true;
		box.append (expander);

		filter_bar = new Gth.FilterBar ();
		filter_bar.valign = Gtk.Align.END;
		box.append (filter_bar);

		return box;
	}

	GLib.Settings browser_settings;
	Gtk.HeaderBar header_bar;
	Gtk.Stack main_stack;
	Gtk.Widget browser_page;
	Gtk.Widget viewer_page;
	Gth.FilterBar filter_bar;
	Page current_page = Page.NONE;
}
