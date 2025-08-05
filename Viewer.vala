[GtkTemplate (ui = "/app/gthumb/gthumb/ui/viewer.ui")]
public class Gth.Viewer : Gtk.Box {
	public weak Window window {
		get { return _window; }
		set {
			_window = value;
			init ();
		}
	}

	public Gth.FileData current_file;
	public int position;

	Gth.Job load_job = null;
	FileViewer current_viewer = null;

	public async void load_file (FileData file, ViewFlags flags = ViewFlags.DEFAULT) throws Error {
		if (load_job != null) {
			load_job.cancel ();
		}
		var local_job = window.new_job ("Load %s".printf (file.file.get_uri ()));
		load_job = local_job;
		try {
			if ((current_viewer == null) || !app.viewer_can_view (current_viewer.get_class ().get_type (), file.get_content_type ())) {
				if (current_viewer != null) {
					before_close_page ();
				}
				current_viewer = app.get_viewer_for_type (file.get_content_type ());
				if (current_viewer == null) {
					throw new IOError.FAILED (_("Cannot load this kind of file"));
				}
				before_open_page ();
				current_viewer.activate (window);
			}
			yield current_viewer.load (file);
			if (!(ViewFlags.KEEP_CURRENT_PAGE in flags)) {
				window.set_page (Window.Page.VIEWER);
			}
			current_file = file;
			property_sidebar.current_file = file;
			update_title ();
			update_sidebar ();
			if (ViewFlags.FULLSCREEN in flags) {
				window.fullscreened = true;
			}
		}
		catch (Error error) {
			stdout.printf ("ERROR: %s\n", error.message);
			local_job.error = error;
		}
		local_job.done ();
		if (load_job == local_job) {
			load_job = null;
		}
		if (local_job.error != null) {
			throw local_job.error;
		}
	}

	Gth.Job sidebar_job = null;

	void update_sidebar () {
		if (sidebar_job != null) {
			sidebar_job.cancel ();
		}
		var local_job = window.new_job ("Load metadata for %s".printf (current_file.file.get_uri ()));
		sidebar_job = local_job;
		property_sidebar.load.begin (current_file, local_job.cancellable, (_obj, res) => {
			try {
				property_sidebar.load.end (res);
			}
			catch (Error error) {
				stdout.printf ("ERROR: %s\n", error.message);
			}
			finally {
				local_job.done ();
				if (sidebar_job == local_job) {
					sidebar_job = null;
				}
			}
		});
	}

	public bool is_loading () {
		return (load_job != null) && load_job.is_running ();
	}

	public void view_file (FileData file, ViewFlags flags = ViewFlags.DEFAULT) {
		load_file.begin (file, flags, (_obj, res) => {
			try {
				load_file.end (res);
			}
			catch (Error error) {
				window.show_error (error);
			}
		});
	}

	public void set_viewer_widget (Gtk.Widget? widget) {
		if (viewer_container.child == widget) {
			return;
		}
		viewer_signals.disconnect_all ();
		viewer_container.child = widget;

		var motion_events = new Gtk.EventControllerMotion ();
		var motion_id = motion_events.motion.connect (on_motion);
		widget.add_controller (motion_events);
		viewer_signals.add (motion_events, motion_id);

		var scroll_events = new Gtk.EventControllerScroll (Gtk.EventControllerScrollFlags.VERTICAL);
		var scroll_id = scroll_events.scroll.connect (on_scroll);
		widget.add_controller (scroll_events);
		viewer_signals.add (scroll_events, scroll_id);
	}

	public void add_viewer_overlay (Gtk.Revealer revealer) {
		viewer_container.add_overlay (revealer);
		overlay_controls.append (revealer);
		add_overlay_motion_controller (revealer);
	}

	public void set_left_toolbar (Gtk.Widget toolbar) {
		left_toolbar.append (toolbar);
	}

	public void set_right_toolbar (Gtk.Widget toolbar) {
		right_toolbar.append (toolbar);
	}

	public void set_statusbar_maximized (bool maximized) {
		statusbar_revealer.halign = maximized ? Gtk.Align.FILL : Gtk.Align.START;
	}

	public void before_open_page () {
		status.set_zoom_info (0);
		status.set_pixel_info (0, 0);
	}

	public void before_close_page () {
		cancel_hide_overlay ();
		Util.remove_all_children (left_toolbar);
		Util.remove_all_children (right_toolbar);
		status.remove_tools ();

		foreach (unowned var revealer in overlay_controls) {
			viewer_container.remove_overlay (revealer);
		}
		overlay_controls = null;

		if (current_viewer != null) {
			current_viewer.deactivate ();
			current_viewer = null;
		}
	}

	public void after_fullscreen () {
		var local_headerbar = headerbar;
		toolbar_view.remove (headerbar);
		headerbar.show_start_title_buttons = false;
		headerbar.show_end_title_buttons = false;
		back_to_browser_button.visible = false;
		fullscreen_toolbar_revealer.child = headerbar;
		fullscreen_button.set_icon_name ("unfullscreen-symbolic");
		fullscreen_state.save ();
	}

	public void after_unfullscreen () {
		var local_headerbar = headerbar;
		fullscreen_toolbar_revealer.child = null;
		toolbar_view.add_top_bar (headerbar);
		headerbar.show_start_title_buttons = true;
		headerbar.show_end_title_buttons = true;
		back_to_browser_button.visible = true;
		fullscreen_button.set_icon_name ("fullscreen-symbolic");
		fullscreen_state.restore ();
	}

	public void save_preferences (bool page_visible) {
		app.viewer_settings.set_boolean (PREF_VIEWER_SIDEBAR_VISIBLE, main_view.show_sidebar);
		if (page_visible) {
			if (current_file != null) {
				app.settings.set_string (PREF_BROWSER_STARTUP_CURRENT_FILE, current_file.file.get_uri ());
			}
			else {
				app.settings.set_string (PREF_BROWSER_STARTUP_CURRENT_FILE, "");
			}
			app.settings.set_int (PREF_BROWSER_PROPERTIES_WIDTH, (int) main_view.max_sidebar_width);
		}
		if (current_viewer != null) {
			current_viewer.deactivate ();
			current_viewer = null;
		}
	}

	void on_motion (double x, double y) {
		if (((last_x - x).abs () < MOTION_THRESHOLD)
				|| ((last_y - y).abs () < MOTION_THRESHOLD))
		{
			return;
		}
		last_x = x;
		last_y = y;
		reveal_overlay_controls ();
	}

	bool on_scroll (double dx, double dy) {
		if (dy < 0) {
			window.browser.view_previous_file ();
		}
		else {
			window.browser.view_next_file ();
		}
		return true;
	}

	public void on_actived_popup (bool active) {
		active_popup = active;
		if (active_popup) {
			cancel_hide_overlay ();
		}
		else {
			hide_overlay_after_timeout ();
		}
	}

	public void update_title () {
		if (current_file != null) {
			window.title = current_file.info.get_display_name ();
		}
	}

	public void set_file_position (uint _position) {
		position = (int) _position;
		status.set_file_position (position);
	}

	public void set_list_info (uint files) {
		status.set_list_info (files);
	}

	void add_overlay_motion_controller (Gtk.Widget revealer) {
		var motion_events = new Gtk.EventControllerMotion ();
		var enter_id = motion_events.enter.connect (() => {
			cancel_hide_overlay ();
			on_overlay = true;
		});
		var leave_id = motion_events.leave.connect (() => {
			on_overlay = false;
			hide_overlay_after_timeout ();
		});
		revealer.add_controller (motion_events);
		viewer_signals.add (motion_events, enter_id);
		viewer_signals.add (motion_events, leave_id);
	}

	public void reveal_overlay_controls (bool reveal = true) {
		if (fullscreen_toolbar_revealer.child != null) {
			fullscreen_toolbar_revealer.reveal_child = reveal;
		}
		statusbar_revealer.reveal_child = reveal;
		foreach (unowned var revealer in overlay_controls) {
			revealer.reveal_child = reveal;
		}
		if (reveal) {
			hide_overlay_after_timeout ();
		}
	}

	void hide_overlay_after_timeout () {
		if (hide_mediabar_id != 0) {
			Source.remove (hide_mediabar_id);
		}
		hide_mediabar_id = Timeout.add_seconds (1, () => {
			hide_mediabar_id = 0;
			if (!on_overlay && !active_popup) {
				reveal_overlay_controls (false);
			}
			return Source.REMOVE;
		});
	}

	void cancel_hide_overlay () {
		if (hide_mediabar_id != 0) {
			Source.remove (hide_mediabar_id);
			hide_mediabar_id = 0;
		}
	}

	void init () {
		current_file = null;
		position = -1;

		property_sidebar.resizer.add_handle (main_view, Gtk.PackType.START);
		property_sidebar.resizer.started.connect ((obj) => {
			window.active_resizer = obj;
		});
		property_sidebar.resizer.ended.connect (() => {
			window.active_resizer = null;
		});

		// Restore settings.
		main_view.show_sidebar = app.viewer_settings.get_boolean (PREF_VIEWER_SIDEBAR_VISIBLE);

		add_overlay_motion_controller (fullscreen_toolbar_revealer);
		init_actions ();
	}

	void init_actions () {
		var builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/app-menu.ui");
		app_menu_button.menu_model = builder.get_object ("app_menu") as MenuModel;

		var action_group = window.action_group;

		var action = new SimpleAction.stateful ("viewer-properties", null, new Variant.boolean (main_view.show_sidebar));
		action.activate.connect ((_action, param) => {
			main_view.show_sidebar = Util.toggle_state (_action);
		});
		action_group.add_action (action);
	}

	construct {
		fullscreen_state = new FullscreenState (this);
		viewer_signals = new RegisteredSignals ();
		add_overlay_motion_controller (statusbar_revealer);
	}

	[GtkChild] public unowned Adw.OverlaySplitView main_view;
	[GtkChild] unowned Gtk.MenuButton app_menu_button;
	[GtkChild] public unowned Gtk.Overlay viewer_container;
	[GtkChild] unowned Gtk.Box left_toolbar;
	[GtkChild] unowned Gtk.Box right_toolbar;
	[GtkChild] public unowned Adw.ToastOverlay toast_overlay;
	[GtkChild] unowned Gth.PropertySidebar property_sidebar;
	[GtkChild] public unowned Gth.ViewerStatus status;
	[GtkChild] unowned Adw.ToolbarView toolbar_view;
	[GtkChild] unowned Gtk.Revealer fullscreen_toolbar_revealer;
	[GtkChild] unowned Gtk.Revealer statusbar_revealer;
	[GtkChild] unowned Adw.HeaderBar headerbar;
	[GtkChild] unowned Gtk.Button back_to_browser_button;
	[GtkChild] unowned Gtk.Button fullscreen_button;

	weak Window _window;
	bool active_popup = false;
	bool on_overlay = false;
	uint hide_mediabar_id = 0;
	double last_x = 0.0;
	double last_y = 0.0;
	List<weak Gtk.Revealer> overlay_controls = null;
	RegisteredSignals viewer_signals;
	FullscreenState fullscreen_state;

	const double MOTION_THRESHOLD = 1.0;
}


class Gth.FullscreenState {
	public FullscreenState (Viewer _viewer) {
		viewer = _viewer;
		sidebar_visibile = false;
	}

	public void save () {
		sidebar_visibile = viewer.main_view.show_sidebar;
		if (sidebar_visibile) {
			var action = viewer.window.action_group.lookup_action ("viewer-properties") as SimpleAction;
			action.set_state (new Variant.boolean (false));
		}
		viewer.main_view.show_sidebar = false;
	}

	public void restore () {
		if (sidebar_visibile != viewer.main_view.show_sidebar) {
			var action = viewer.window.action_group.lookup_action ("viewer-properties") as SimpleAction;
			action.activate (null);
		}
	}

	weak Viewer viewer;
	bool sidebar_visibile;
}
