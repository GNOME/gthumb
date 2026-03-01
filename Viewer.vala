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
	public FileViewer current_viewer = null;
	public int position;
	public int toasts = 0;

	async bool load_file_async (FileData file_data, ViewFlags flags, Job job) throws Error {
		// Ask to save the current file if modified
		if ((current_file != null) && current_file.get_is_modified ()) {
			yield ask_whether_to_save (job);
		}
		activate_viewer_for_file (file_data);
		var loaded = yield current_viewer.load (file_data, job);
		yield window.set_page (Window.Page.VIEWER);
		current_file = file_data;
		property_sidebar.current_file = current_file;
		// Do not update the sidebar when loading images
		// the file info is already updated by the ImageLoader.
		if (!(loaded && (current_viewer is ImageViewer))) {
			update_sidebar ();
		}
		if (loaded && (ViewFlags.FULLSCREEN in flags)) {
			window.fullscreened = true;
		}
		update_title ();
		update_sensitivity ();
		current_viewer.update_sensitivity ();
		if (ViewFlags.FOCUS in flags) {
			focus_viewer ();
		}
		return loaded;
	}

	Job load_job = null;

	public async bool view_file_async (FileData file_data, ViewFlags flags = ViewFlags.DEFAULT, Job? job = null, uint new_position = 0) {
		if (load_job != null) {
			load_job.cancel ();
		}
		if (ViewFlags.WITH_NEW_POSITION in flags) {
			position = (int) new_position;
		}
		else {
			position = window.browser.get_file_position (file_data.file);
		}
		set_current_position (position);
		var local_job = (job != null) ? job : window.new_job (_("Loading %s").printf (file_data.get_display_name ()),
			JobFlags.FOREGROUND,
			"gth-content-loading-symbolic");
		load_job = local_job;
		var loaded = false;
		var cancelled = false;
		try {
			loaded = yield load_file_async (file_data, flags, local_job);
		}
		catch (Error error) {
			if (error is IOError.CANCELLED) {
				cancelled = true;
			}
		}
		if (window.browser.never_loaded) {
			yield window.browser.first_load ();
			flags |= ViewFlags.LOAD_PARENT_DIRECTORY;
		}
		var parent = file_data.file.get_parent ();
		if ((ViewFlags.LOAD_PARENT_DIRECTORY in flags)
			&& ((window.browser.folder_tree.current_folder == null)
				|| !window.browser.folder_tree.current_folder.file.equal (parent)))
		{
			try {
				yield window.browser.load_folder (parent, LoadAction.OPEN);
			}
			catch (Error error) {
				window.show_error (error);
			}
		}
		if (!cancelled) {
			if (current_viewer != null) {
				current_viewer.preload_some_files ();
			}
		}
		if (local_job != job) {
			local_job.done ();
		}
		if (load_job == local_job) {
			load_job = null;
		}
		return loaded;
	}

	public async bool open_file_async (File file, ViewFlags flags, Job job) throws Error {
		var attributes = window.browser.get_list_attributes ();
		var file_data = yield FileManager.read_metadata (file, attributes, job.cancellable);
		return yield view_file_async (file_data, flags, job);
	}

	public void reload () {
		if (current_file == null) {
			return;
		}
		view_file_async.begin (current_file);
	}

	public void file_renamed (FileData file_data) {
		current_file = file_data;
		property_sidebar.current_file = current_file;
		update_sidebar ();
		update_title ();
		update_sensitivity ();
		update_current_file_position ();
	}

	public async void open_unsaved_image (FileData file_data, Gth.Image image) {
		if (load_job != null) {
			load_job.cancel ();
		}
		var local_job = window.new_job (_("Loading %s").printf (file_data.get_display_name ()),
			JobFlags.FOREGROUND,
			"gth-content-loading-symbolic");
		load_job = local_job;
		try {
			activate_viewer_for_file (file_data);
			var image_viewer = current_viewer as ImageViewer;
			if (image_viewer == null) {
				throw new IOError.FAILED (_("Cannot view this kind of files"));
			}
			image_viewer.view_unsaved_image (image);
			yield window.set_page (Window.Page.VIEWER);
			current_file = file_data;
			property_sidebar.current_file = file_data;
			update_title ();
			update_sensitivity ();
			image_viewer.update_sensitivity ();
		}
		catch (Error error) {
			window.show_error (error);
		}
		local_job.done ();
		if (load_job == local_job) {
			load_job = null;
		}
	}

	public async void ask_whether_to_save (Job? job = null) throws Error {
		var dialog = new Adw.AlertDialog (
			_("File Modified"),
			_("If you don’t save, changes to the file will be permanently lost.")
		);
		dialog.add_responses (
			"cancel",  _("_Cancel"),
			"discard", _("_Discard"),
			"save", _("_Save")
		);
		dialog.set_response_appearance ("discard", Adw.ResponseAppearance.DESTRUCTIVE);
		dialog.set_response_appearance ("save", Adw.ResponseAppearance.SUGGESTED);
		dialog.default_response = "save";
		dialog.close_response = "cancel";
		if (job != null) {
			job.opens_dialog ();
		}
		var response = yield dialog.choose (window, null);
		if (job != null) {
			job.dialog_closed ();
		}
		if (response == "cancel") {
			throw new IOError.CANCELLED ("Cancelled");
		}
		if (response == "discard") {
			if (current_file.get_is_modified ()) {
				Lib.restore_original_attribute (current_file.info, "Frame::Size");
				Lib.restore_original_attribute (current_file.info, "Frame::Width");
				Lib.restore_original_attribute (current_file.info, "Frame::Height");
				current_file.set_is_modified (false);
			}
			return;
		}
		yield current_viewer.save ();
	}

	public void after_saving (FileData file_data) {
		current_file.set_is_modified (false);
		update_title ();
		app.events.file_saved (file_data.file);
	}

	public void file_changed (FileData file_data) {
		// stdout.printf ("> VIEWER: FILE CHANGED %s\n", file_data.file.get_uri ());
		if ((current_file != null) && current_file.file.equal (file_data.file)) {
			if (!current_file.get_is_modified ()
				&& !current_viewer.same_etag (file_data.info))
			{
				// stdout.printf ("> VIEWER: RELOAD FILE %s\n", file_data.file.get_uri ());
				view_file_async.begin (file_data, ViewFlags.DEFAULT);
			}
		}
	}

	public void list_changed () {
		update_current_file_position ();
	}

	void update_current_file_position () {
		if (current_file == null) {
			return;
		}
		var new_position = window.browser.get_file_position (current_file.file);
		set_current_position (new_position);
		if (new_position >= 0) {
			position = new_position;
		}
	}

	void set_current_position (int pos) {
		status.set_file_position (pos);
		file_grid.select_position (pos, SelectFile.SCROLL_TO_FILE);
	}

	public void metadata_changed (File file) {
		if ((current_file != null) && current_file.file.equal (file)) {
			// stdout.printf ("> VIEWER: RELOAD METADATA %s\n", file.get_uri ());
			update_sidebar ();
		}
	}

	void activate_viewer_for_file (FileData file) {
		if ((current_viewer == null) || !app.viewer_can_view (current_viewer.get_class ().get_type (), file.get_content_type ())) {
			if (current_viewer != null) {
				before_close_page ();
			}
			current_viewer = app.get_viewer_for_type (file.get_content_type ());
			before_open_page ();
			current_viewer.activate (window);
		}
	}

	Gth.Job sidebar_job = null;

	void update_sidebar () {
		if (sidebar_job != null) {
			sidebar_job.cancel ();
		}
		var local_job = window.new_job ("Metadata for %s".printf (current_file.get_display_name ()),
			JobFlags.DEFAULT,
			"gth-note-symbolic");
		sidebar_job = local_job;
		property_sidebar.load.begin (current_file, local_job.cancellable, (_obj, res) => {
			try {
				property_sidebar.load.end (res);
			}
			catch (Error error) {
				stdout.printf ("ERROR: Viewer.update_sidebar: %s\n", error.message);
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

	uint view_timeout = 0;

	public void view_file (FileData file, ViewFlags flags = ViewFlags.DEFAULT, uint new_position = 0) {
		// TODO
		//if (view_timeout != 0) {
		//	Source.remove (view_timeout);
		//	view_timeout = 0;
		//}
		//if (!(ViewFlags.NO_DELAY in flags)) {
		//	view_timeout = Util.after_timeout (150, () => {
		//		view_file (file, flags | ViewFlags.NO_DELAY);
		//	});
		//	return;
		//}
		view_file_async.begin (file, flags, null, new_position);
	}

	public void show_editor_tools (bool show) {
		if (show) {
			sidebar_stack.set_visible_child (editor_palette);
			window.action_group.change_action_state ("viewer-properties", new Variant.boolean (false));
		}
		var action = window.action_group.lookup_action ("editor-tools") as SimpleAction;
		action.set_state (new Variant.boolean (show));
		main_view.show_sidebar = show;
	}

	public void set_viewer_widget (Gtk.Widget? widget) {
		if (viewer_container.child == widget) {
			return;
		}
		viewer_signals.disconnect_all ();
		viewer_container.child = widget;

		if (widget == null) {
			return;
		}

		var motion_events = new Gtk.EventControllerMotion ();
		var motion_id = motion_events.motion.connect (on_motion);
		widget.add_controller (motion_events);
		viewer_signals.add (motion_events, motion_id);

		var scroll_events = new Gtk.EventControllerScroll (Gtk.EventControllerScrollFlags.VERTICAL);
		var scroll_id = scroll_events.scroll.connect (on_scroll);
		widget.add_controller (scroll_events);
		viewer_signals.add (scroll_events, scroll_id);

		var click_events = new Gtk.GestureClick ();
		click_events.set_button (Gdk.BUTTON_SECONDARY);
		var click_id = click_events.pressed.connect ((n_press, x, y) => {
			context_menu.pointing_to = { (int) x, (int) y, 1, 24 };
			context_menu.popup ();
		});
		widget.add_controller (click_events);
		viewer_signals.add (click_events, click_id);

		click_events = new Gtk.GestureClick ();
		click_id = click_events.pressed.connect (() => focus_viewer ());
		widget.add_controller (click_events);
		viewer_signals.add (click_events, click_id);
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

	public void before_open_page () {
	}

	public void before_close_page () {
		viewer_signals.disconnect_all ();
		viewer_container.child = null;
		Util.remove_all_children (left_toolbar);
		Util.remove_all_children (right_toolbar);
		set_mediabar (null);

		foreach (unowned var revealer in overlay_controls) {
			viewer_container.remove_overlay (revealer);
		}
		overlay_controls = null;

		if (current_viewer != null) {
			current_viewer.deactivate ();
			current_viewer.save_preferences ();
			current_viewer.release_resources ();
			current_viewer = null;
		}
	}

	public void after_fullscreen () {
		var local_headerbar = headerbar;
		toolbar_view.remove (headerbar);
		local_headerbar.show_start_title_buttons = false;
		local_headerbar.show_end_title_buttons = false;
		back_to_browser_button.visible = false;
		fullscreen_toolbar_revealer.child = local_headerbar;
		statusbar_revealer.margin_top = 72;
		fullscreen_button.icon_name = "gth-unfullscreen-symbolic";
		fullscreen_button.tooltip_text = _("Exit Fullscreen");
		adapt_window_button.visible = false;
		fullscreen_state.save ();
		reveal_overlay_controls (false);
	}

	public void after_unfullscreen () {
		var local_headerbar = headerbar;
		fullscreen_toolbar_revealer.child = null;
		toolbar_view.add_top_bar (local_headerbar);
		local_headerbar.show_start_title_buttons = true;
		local_headerbar.show_end_title_buttons = true;
		back_to_browser_button.visible = true;
		statusbar_revealer.margin_top = 24;
		fullscreen_button.icon_name = "gth-fullscreen-symbolic";
		fullscreen_button.tooltip_text = _("Fullscreen");
		adapt_window_button.visible = true;
		fullscreen_state.restore ();
		cursor = null;
	}

	public void save_preferences (bool page_visible) {
		app.viewer_settings.set_boolean (PREF_VIEWER_SIDEBAR_VISIBLE, main_view.show_sidebar && !main_view.collapsed);
		app.viewer_settings.set_boolean (PREF_VIEWER_FILE_LIST_VISIBLE, content_view.show_sidebar && !content_view.collapsed);
		app.viewer_settings.set_int (PREF_VIEWER_FILE_LIST_SIZE, int.min ((int) content_view.max_sidebar_width, MAX_SIDEBAR_WIDTH));
		if (page_visible) {
			if (current_file != null) {
				app.settings.set_string (PREF_BROWSER_SESSION_CURRENT_FILE, current_file.file.get_uri ());
			}
			else {
				app.settings.set_string (PREF_BROWSER_SESSION_CURRENT_FILE, "");
			}
			app.settings.set_int (PREF_BROWSER_PROPERTIES_WIDTH, int.min ((int) main_view.max_sidebar_width, MAX_SIDEBAR_WIDTH));
		}
		if (current_viewer != null) {
			current_viewer.save_preferences ();
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

	bool on_scroll (Gtk.EventController controller, double dx, double dy) {
		if (current_viewer == null) {
			return false;
		}
		var state = controller.get_current_event_state ();
		return current_viewer.on_scroll (dx, dy, state);
	}

	public bool on_scroll_change_file (double dx, double dy) {
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

	public void update_sensitivity () {
		var is_image = (current_file != null) && Util.content_type_is_image (current_file.get_content_type ());
		Util.enable_action (window.action_group, "print", is_image);
		Util.enable_action (window.action_group, "set-desktop-background", is_image);
		Util.enable_action (window.action_group, "editor-tools", is_image);
	}

	public void update_title () {
		var title = new StringBuilder ();
		string state = null;
		if (current_file != null) {
			title.append (current_file.info.get_display_name ());
			if (current_file.get_is_modified ()) {
				// Translators: text added to the window title when the image is modified and unsaved.
				state = _("modified");
			}
		}
		if (window.current_page == Window.Page.VIEWER) {
			window.title = title.str;
		}
		set_header_state (state);
	}

	public void update_list_info (uint files) {
		status.set_list_info (files);
		update_current_file_position ();
	}

	public GenericList<File> get_selected_file () {
		var selected_files = new GenericList<File>();
		if (current_file != null) {
			selected_files.model.append (current_file.file);
		}
		return selected_files;
	}

	public GenericList<FileData> get_selected_file_data_list () {
		var selected_files = new GenericList<FileData>();
		if (current_file != null) {
			selected_files.model.append (current_file);
		}
		return selected_files;
	}

	public void files_deleted_from_disk (GenericList<File> files) {
		if (current_file == null) {
			return;
		}
		var iter = files.iterator ();
		var deleted_pos = iter.find_first ((file) => file.equal (current_file.file));
		if (deleted_pos < 0) {
			return;
		}
		if (window.current_page == Window.Page.BROWSER) {
			current_file = null;
		}
		else if (!window.browser.view_position (position)) {
			if (!window.browser.view_last_file ()) {
				window.set_page.begin (Window.Page.BROWSER);
			}
		}
	}

	void add_overlay_motion_controller (Gtk.Widget revealer, bool removable = true) {
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
		if (removable) {
			viewer_signals.add (motion_events, enter_id);
			viewer_signals.add (motion_events, leave_id);
		}
		else {
			fixed_signals.add (motion_events, enter_id);
			fixed_signals.add (motion_events, leave_id);
		}
	}

	public void reveal_overlay_controls (bool reveal = true, bool hide_after_timeout = true) {
		if (fullscreen_toolbar_revealer.child != null) {
			fullscreen_toolbar_revealer.reveal_child = reveal;
		}
		statusbar_revealer.reveal_child = reveal;
		mediabar_revealer.reveal_child = reveal;
		foreach (unowned var revealer in overlay_controls) {
			revealer.reveal_child = reveal;
		}
		if (window.fullscreened) {
			cursor = reveal ? null : new Gdk.Cursor.from_name ("none", null);
		}
		if (reveal && hide_after_timeout) {
			hide_overlay_after_timeout ();
		}
	}

	public void set_mediabar (Gtk.Widget? widget, Gtk.Align halign = Gtk.Align.FILL) {
		if (widget != null) {
			mediabar_revealer.set_child (widget);
			mediabar_revealer.visible = true;
			mediabar_revealer.halign = halign;
		}
		else {
			mediabar_revealer.set_child (null);
			mediabar_revealer.visible = false;
		}
	}

	public void set_header_state (string? state) {
		if (state != null) {
			header_state.label = state;
			header_state.visible = true;
		}
		else {
			header_state.visible = false;
		}
	}

	public void release_resources () {
		viewer_signals.disconnect_all ();
		fixed_signals.disconnect_all ();
		if (view_timeout != 0) {
			Source.remove (view_timeout);
			view_timeout = 0;
		}
		cancel_hide_overlay ();
		if (current_viewer != null) {
			current_viewer.release_resources ();
		}
	}

	public void focus_viewer () {
		if (window.current_page != Window.Page.VIEWER) {
			return;
		}
		if (current_viewer != null) {
			current_viewer.focus ();
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
		window.bind_property ("title", header_title, "title", BindingFlags.SYNC_CREATE);

		property_sidebar.resizer.add_handle (main_view, Gtk.PackType.START);
		property_sidebar.resizer.started.connect ((obj) => {
			window.active_resizer = obj;
		});
		property_sidebar.resizer.ended.connect (() => {
			window.active_resizer = null;
		});

		editor_palette.resizer.add_handle (main_view, Gtk.PackType.START);
		editor_palette.resizer.started.connect ((obj) => {
			window.active_resizer = obj;
		});
		editor_palette.resizer.ended.connect (() => {
			window.active_resizer = null;
		});

		file_grid_resizer.add_handle (content_view, Gtk.PackType.END);
		file_grid_resizer.started.connect ((obj) => {
			window.active_resizer = obj;
		});
		file_grid_resizer.ended.connect (() => {
			window.active_resizer = null;
		});
		file_grid_resizer.add_css_class ("view");

		file_grid.set_model (window.browser.file_grid.view.model, true);
		file_grid.view.model.selection_changed.connect (() => {
			var single_selection = file_grid.view.model as Gtk.SingleSelection;
			var pos = single_selection.get_selected ();
			if ((pos != uint.MAX) && (pos != position)) {
				window.browser.view_position (pos);
			}
		});

		// Restore settings.
		main_view.show_sidebar = app.viewer_settings.get_boolean (PREF_VIEWER_SIDEBAR_VISIBLE);
		file_grid.thumbnailer = new Thumbnailer.for_window (window);
		file_grid.thumbnail_size = app.settings.get_int (PREF_BROWSER_THUMBNAIL_SIZE);
		content_view.max_sidebar_width = app.viewer_settings.get_int (PREF_VIEWER_FILE_LIST_SIZE);
		content_view.show_sidebar = app.viewer_settings.get_boolean (PREF_VIEWER_FILE_LIST_VISIBLE);

		add_overlay_motion_controller (fullscreen_toolbar_revealer, false);
		add_overlay_motion_controller (statusbar_revealer, false);
		add_overlay_motion_controller (mediabar_revealer, false);
		init_actions ();
	}

	void init_actions () {
		var builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/viewer-menu.ui");
		app_menu_button.menu_model = builder.get_object ("app_menu") as MenuModel;

		var action_group = window.action_group;

		var action = new SimpleAction.stateful ("viewer-properties", null, new Variant.boolean (main_view.show_sidebar));
		action.activate.connect ((_action, param) => {
			var view_properties = Util.toggle_state (_action);
			if (view_properties) {
				sidebar_stack.set_visible_child (property_sidebar);
				action_group.change_action_state ("editor-tools", new Variant.boolean (false));
			}
			main_view.show_sidebar = view_properties;
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("editor-tools", null, new Variant.boolean (false));
		action.activate.connect ((_action, param) => {
			show_editor_tools (Util.toggle_state (_action));
		});
		action_group.add_action (action);

		action = new SimpleAction ("adapt-to-content", null);
		action.activate.connect ((_action, param) => {
			uint width, height;
			if (current_viewer.get_pixel_size (out width, out height)) {
				var extra_width = (uint) (main_view.show_sidebar ? main_view.sidebar.get_width () : 0);
				var extra_height = (uint) headerbar.get_height ();

				//stdout.printf ("> WINDOW SIZE: %dx%d\n", window.get_width (), window.get_height ());

				int monitor_width, monitor_height;
				window.monitor_profile.get_geometry (out monitor_width, out monitor_height);
				//stdout.printf ("  MONITOR: %dx%d\n", monitor_width, monitor_height);

				uint max_width = (uint) ((double) monitor_width * 0.8);
				uint max_height = (uint) ((double) monitor_height * 0.8);

				var new_width = width;
				var new_height = height;
				Lib.scale_keeping_ratio (ref new_width, ref new_height,
					max_width - extra_width,
					max_height - extra_height,
					true);
				new_width += extra_width;
				new_height += extra_height;
				//stdout.printf ("  NEW SIZE: %ux%u\n", new_width, new_height);

				window.set_default_size ((int) new_width, (int) new_height);
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("viewer-controls", null);
		action.activate.connect ((_action, param) => {
			reveal_overlay_controls (!statusbar_revealer.reveal_child, false);
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("viewer-thumbnail-list", null, new Variant.boolean (content_view.show_sidebar));
		action.activate.connect ((_action, param) => {
			content_view.show_sidebar = Util.toggle_state (_action);
		});
		action_group.add_action (action);
	}

	construct {
		fullscreen_state = new FullscreenState (this);
		viewer_signals = new RegisteredSignals ();
		fixed_signals = new RegisteredSignals ();
	}

	[GtkChild] public unowned Adw.OverlaySplitView main_view;
	[GtkChild] public unowned Gtk.MenuButton app_menu_button;
	[GtkChild] public unowned Gtk.MenuButton scripts_menu_button;
	[GtkChild] public unowned Gtk.Overlay viewer_container;
	[GtkChild] unowned Gtk.Box left_toolbar;
	[GtkChild] unowned Gtk.Box right_toolbar;
	[GtkChild] public unowned Gth.PropertySidebar property_sidebar;
	[GtkChild] public unowned Gth.EditorPalette editor_palette;
	[GtkChild] public unowned Gth.ViewerStatus status;
	[GtkChild] unowned Adw.ToolbarView toolbar_view;
	[GtkChild] unowned Gtk.Revealer fullscreen_toolbar_revealer;
	[GtkChild] unowned Gtk.Revealer statusbar_revealer;
	[GtkChild] unowned Gtk.Revealer mediabar_revealer;
	[GtkChild] unowned Adw.HeaderBar headerbar;
	[GtkChild] unowned Gtk.Button back_to_browser_button;
	[GtkChild] unowned Gtk.Button fullscreen_button;
	[GtkChild] unowned Gtk.Button adapt_window_button;
	[GtkChild] unowned Gtk.PopoverMenu context_menu;
	[GtkChild] unowned Adw.WindowTitle header_title;
	[GtkChild] unowned Gtk.Label header_state;
	[GtkChild] public unowned Gth.ActionPopover tools_popover;
	[GtkChild] public unowned Gtk.Stack sidebar_stack;
	[GtkChild] public unowned Adw.OverlaySplitView content_view;
	[GtkChild] public unowned Gth.FileGrid file_grid;
	[GtkChild] public unowned Gth.SidebarResizer file_grid_resizer;

	weak Window _window;
	bool active_popup = false;
	bool on_overlay = false;
	uint hide_mediabar_id = 0;
	double last_x = 0.0;
	double last_y = 0.0;
	List<weak Gtk.Revealer> overlay_controls = null;
	RegisteredSignals viewer_signals;
	RegisteredSignals fixed_signals;
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
			Util.set_active (viewer.window.action_group, "viewer-properties", false);
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
