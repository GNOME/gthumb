[GtkTemplate (ui = "/app/gthumb/gthumb/ui/viewer.ui")]
public class Gth.Viewer : Gtk.Box {
	public enum ViewFlags {
		DEFAULT,
		KEEP_CURRENT_PAGE,
		NO_DELAY,
		FULLSCREEN,
	}

	public weak Window window {
		get { return _window; }
		set {
			_window = value;
			init ();
		}
	}

	public Gth.FileData current_file;

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
					cancel_hide_overlay ();
					current_viewer.deactivate ();
				}
				current_viewer = app.get_viewer_for_type (file.get_content_type ());
				if (current_viewer == null) {
					throw new IOError.FAILED (_("Cannot load this kind of file"));
				}
				current_viewer.activate (window);
			}
			yield current_viewer.load (file);
			if (!(ViewFlags.KEEP_CURRENT_PAGE in flags)) {
				window.set_page (Window.Page.VIEWER);
			}
			property_sidebar.file_data = window.browser.property_sidebar.file_data;
			current_file = file;
			update_title ();
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
		if (viewer_container.child != null) {
			viewer_container.child.destroy ();
		}
		viewer_container.child = widget;
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

	public void before_close_page () {
		cancel_hide_overlay ();
		Util.remove_all_children (left_toolbar);
		Util.remove_all_children (right_toolbar);

		foreach (unowned var revealer in overlay_controls) {
			viewer_container.remove_overlay (revealer);
		}
		overlay_controls = null;

		if (current_viewer != null) {
			current_viewer.deactivate ();
			current_viewer = null;
		}
	}

	public void after_show_page () {
		if (current_viewer != null) {
			current_viewer.show ();
		}
	}

	public void after_fullscreen () {
		var local_headerbar = headerbar;
		toolbar_view.remove (headerbar);
		headerbar.show_start_title_buttons = false;
		headerbar.show_end_title_buttons = false;
		fullscreen_toolbar_revealer.child = headerbar;
		fullscreen_state.save ();
	}

	public void after_unfullscreen () {
		var local_headerbar = headerbar;
		fullscreen_toolbar_revealer.child = null;
		toolbar_view.add_top_bar (headerbar);
		headerbar.show_start_title_buttons = true;
		headerbar.show_end_title_buttons = true;
		fullscreen_state.restore ();
	}

	public void on_motion (double x, double y) {
		if (((last_x - x).abs () < MOTION_THRESHOLD)
				|| ((last_y - y).abs () < MOTION_THRESHOLD))
		{
			return;
		}
		last_x = x;
		last_y = y;
		reveal_overlay_controls (true);
		hide_overlay_after_timeout ();
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

	public void set_file_position (uint position) {
		file_position.label = "%u".printf (position + 1);
	}

	public void set_list_info (uint files) {
		total_files.label = " / %u".printf (files);
	}

	void add_overlay_motion_controller (Gtk.Widget revealer) {
		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.enter.connect (() => {
			cancel_hide_overlay ();
			on_overlay = true;
		});
		motion_events.leave.connect (() => {
			on_overlay = false;
			hide_overlay_after_timeout ();
		});
		revealer.add_controller (motion_events);
	}

	void reveal_overlay_controls (bool reveal) {
		if (fullscreen_toolbar_revealer.child != null) {
			fullscreen_toolbar_revealer.reveal_child = reveal;
		}
		foreach (unowned var revealer in overlay_controls) {
			revealer.reveal_child = reveal;
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

		sidebar_resizer.add_handle (main_view, Gtk.PackType.START);
		sidebar_resizer.started.connect ((obj) => {
			window.active_resizer = obj;
		});
		sidebar_resizer.ended.connect (() => {
			window.active_resizer = null;
		});

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
	}

	[GtkChild] public unowned Adw.OverlaySplitView main_view;
	[GtkChild] unowned Gth.SidebarResizer sidebar_resizer;
	[GtkChild] unowned Gtk.MenuButton app_menu_button;
	[GtkChild] public unowned Gtk.Overlay viewer_container;
	[GtkChild] unowned Gtk.Box left_toolbar;
	[GtkChild] unowned Gtk.Box right_toolbar;
	[GtkChild] public unowned Adw.ToastOverlay toast_overlay;
	[GtkChild] unowned Gth.PropertySidebar property_sidebar;
	[GtkChild] public unowned Gth.ViewerStatus status;
	[GtkChild] unowned Gtk.Label total_files;
	[GtkChild] unowned Gtk.Label file_position;
	[GtkChild] unowned Adw.ToolbarView toolbar_view;
	[GtkChild] unowned Gtk.Revealer fullscreen_toolbar_revealer;
	[GtkChild] unowned Adw.HeaderBar headerbar;

	weak Window _window;
	bool active_popup = false;
	bool on_overlay = false;
	uint hide_mediabar_id = 0;
	double last_x = 0.0;
	double last_y = 0.0;
	List<weak Gtk.Revealer> overlay_controls = null;
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
