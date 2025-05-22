public class Gth.Window : Adw.ApplicationWindow {
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
			var dialog = new Gth.PersonalizeFilters ();
			dialog.present (this);
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("set-filter", GLib.VariantType.STRING, new Variant.string (""));
		action.activate.connect ((_action, param) => {
			_action.set_state (param);
			filter_bar.select_filter_by_id (param.get_string ());
		});
		action_group.add_action (action);
	}

	HashTable<string, Gtk.Window> named_dialogs;

	public Window (Gtk.Application _app, File location, File? file_to_select) {
		Object (application: app);

		named_dialogs = new HashTable<string, Gtk.Window>(str_hash, str_equal);
		init_actions ();

		// Content

		var toolbar_view = new Adw.ToolbarView ();
		content = toolbar_view;

		var header_bar = new Adw.HeaderBar ();
		toolbar_view.add_top_bar (header_bar);

		// Pages

		main_stack = new Gtk.Stack ();
		main_stack.transition_type = Gtk.StackTransitionType.CROSSFADE;
		toolbar_view.content = main_stack;

		browser_page = build_browser_page ();
		main_stack.add_child (browser_page);

		viewer_page = new Gtk.Label ("VIEWER");
		main_stack.add_child (viewer_page);

		set_page (Page.BROWSER);

		// Restore the window size.

		var width = app.browser_settings.get_int (PREF_BROWSER_WINDOW_WIDTH);
		var height = app.browser_settings.get_int (PREF_BROWSER_WINDOW_HEIGHT);
		set_default_size (width, height);

		if (app.browser_settings.get_boolean (PREF_BROWSER_WINDOW_MAXIMIZED)) {
			maximize ();
		}

		// Load the location.

		title = "Thumbnails";
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

	public void set_named_dialog (string name, Gtk.Window dialog) {
		dialog.close_request.connect (() => {
			named_dialogs[name] = null;
			return false;
		});
		named_dialogs[name] = dialog;
	}

	public Gtk.Window get_named_dialog (string name) {
		return named_dialogs[name];
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
		expander.add_css_class ("deep-view");
		box.append (expander);

		filter_bar = new Gth.FilterBar ();
		filter_bar.valign = Gtk.Align.END;
		box.append (filter_bar);

		return box;
	}

	Gtk.Stack main_stack;
	Gtk.Widget browser_page;
	Gtk.Widget viewer_page;
	Gth.FilterBar filter_bar;
	Page current_page = Page.NONE;
}
