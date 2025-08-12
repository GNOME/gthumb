[GtkTemplate (ui = "/app/gthumb/gthumb/ui/window.ui")]
public class Gth.Window : Adw.ApplicationWindow {
	public Gth.JobQueue jobs;
	public bool closing;
	public SimpleActionGroup action_group;
	public FileManager file_manager;

	public enum Page {
		NONE = 0,
		BROWSER,
		VIEWER,
	}

	construct {
		title = "Thumbnails";

		jobs = new Gth.JobQueue ();
		jobs.size_changed.connect (() => {
			if (closing && (jobs.size () == 0)) {
				before_closing ();
				close ();
			}
		});
		file_manager = new FileManager (this);
		closing = false;
		action_group = new SimpleActionGroup ();
		insert_action_group ("win", action_group);

		close_request.connect (() => {
			if (file_not_saved ()) {
				save_and_close.begin ();
				return true;
			}
			closing = true;
			if (jobs.size () > 0) {
				jobs.cancel_all ();
				return true;
			}
			before_closing ();
			return false;
		});

		active_resizer = null;

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((x, y) => {
			if (active_resizer != null) {
				active_resizer.update_width (x);
			}
		});
		child.add_controller (motion_events);

		unowned var clipboard = get_clipboard ();
		clipboard_event = clipboard.changed.connect (() => {
			update_sensitivity_for_clipboard ();
		});

		init_actions ();
		browser.window = this;
		viewer.window = this;
	}

	ulong clipboard_event = 0;

	public Window (Gtk.Application _app) {
		Object (application: app);
	}

	public void add_toast (Adw.Toast toast) {
		if (current_page == Page.BROWSER) {
			browser.toast_overlay.add_toast (toast);
		}
		else if (current_page == Page.VIEWER) {
			viewer.toast_overlay.add_toast (toast);
		}
	}

	public void show_message (string message) {
		add_toast (Util.new_literal_toast (message));
	}

	public void show_error (Error error) {
		if (error is IOError.CANCELLED)
			return;
		var toast = Util.new_literal_toast (error.message);
		toast.priority = Adw.ToastPriority.HIGH;
		add_toast (toast);
	}

	public void edge_reached (string? msg = null) {
		if (msg != null) {
			show_message (msg);
		}
		else {
			get_surface ().beep ();
		}
	}

	public void copy_text_to_clipboard (string text) {
		var clipboard = get_clipboard ();
		clipboard.set_text (text);
		show_message (_("Copied to Clipboard"));
	}

	bool file_not_saved () {
		return (current_page == Page.VIEWER) && viewer.current_file.get_is_modified ();
	}

	async void save_and_close () {
		try {
			yield viewer.ask_whether_to_save ();
		}
		catch (Error error) {
			show_error (error);
			return;
		}
		close ();
	}

	public async void set_page (Page new_page) {
		if (new_page == current_page) {
			return;
		}
		if ((current_page == Page.VIEWER) && viewer.current_file.get_is_modified ()) {
			try {
				yield viewer.ask_whether_to_save ();
			}
			catch (Error error) {
				show_error (error);
				return;
			}
		}
		var previuos_page = current_page;
		current_page = new_page;
		switch (current_page) {
		case Page.BROWSER:
			if (!browser.never_loaded && (previuos_page == Page.VIEWER)) {
				//if (viewer.main_view.show_sidebar) {
				//	browser.content_view.max_sidebar_width = viewer.main_view.max_sidebar_width;
				//}
				viewer.before_close_page ();
				browser.restore_window_size ();
				if ((viewer.current_file != null) && viewer.current_file.file.equal (browser.property_sidebar.current_file.file)) {
					browser.property_sidebar.update_view ();
				}
				else if (viewer.position >= 0) {
					browser.select_position (viewer.position);
				}
				else if (viewer.current_file != null) {
					browser.select_file (viewer.current_file.file);
				}
			}
			stack.set_visible_child (browser);
			if (browser.never_loaded) {
				yield browser.first_load ();
				if (previuos_page == Page.VIEWER) {
					if (viewer.current_file != null) {
						browser.open_location (
							viewer.current_file.file.get_parent (),
							LoadAction.OPEN,
							viewer.current_file.file);
					}
				}
			}
			browser.update_title ();
			break;
		case Page.VIEWER:
			if (previuos_page == Page.BROWSER) {
				browser.save_window_size ();
				if (browser.content_view.show_sidebar) {
					viewer.main_view.max_sidebar_width = browser.content_view.max_sidebar_width;
				}
			}
			stack.set_visible_child (viewer);
			viewer.update_title ();
			break;
		}
		update_sensitivity ();
	}

	public Gth.Job new_job (string description, JobFlags flags = JobFlags.DEFAULT) {
		var job = app.new_job (description, flags);
		add_job (job);
		return job;
	}

	public void with_new_job (string description, JobFlags flags, JobFunc func) {
		var local_job = new_job (description, flags);
		try {
			func (local_job);
		}
		catch (Error error) {
			show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	public void add_job (Gth.Job job) {
		jobs.add_job (job);
		if (job.foreground) {
			var toast = Util.new_literal_toast (job.description);
			toast.button_label = _("_Cancel");
			toast.action_name = "win.cancel-job";
			toast.action_target = new Variant.uint64 (job.id);
			toast.priority = Adw.ToastPriority.HIGH;
			toast.timeout = 0;
			job.toast = toast;
			add_toast (toast);
		}
	}

	public void cancel_jobs () {
		jobs.cancel_all ();
	}

	public void on_setting_change (string key) {
		switch (key) {
		case PREF_BROWSER_THUMBNAIL_SIZE:
			browser.set_thumbnail_size (app.settings.get_int (PREF_BROWSER_THUMBNAIL_SIZE));
			break;
		case PREF_BROWSER_THUMBNAIL_CAPTION:
			browser.reload ();
			break;
		case PREF_BROWSER_OPEN_IN_FULLSCREEN:
			browser.open_in_fullscreen = app.settings.get_boolean (PREF_BROWSER_OPEN_IN_FULLSCREEN);
			break;
		}
	}

	public string? get_monitor_name () {
		unowned var display = get_display ();
		unowned var monitor = display.get_monitor_at_surface (get_surface ());
		if (monitor == null) {
			return null;
		}
		return monitor.description;
	}

	IccProfile monitor_profile = null;
	bool no_monitor_profile = false;

	public async IccProfile get_monitor_profile (Cancellable cancellable) {
		if (no_monitor_profile) {
			return null;
		}
		if (monitor_profile != null) {
			return monitor_profile;
		}
		unowned var display = get_display ();
		unowned var monitor = display.get_monitor_at_surface (get_surface ());
		if (monitor == null) {
			no_monitor_profile = true;
			return null;
		}
		try {
			monitor_profile = yield app.color_manager.get_profile_async (monitor.description, cancellable);
		}
		catch (Error error) {
			if (!(error is IOError.CANCELLED)) {
				no_monitor_profile = true;
			}
		}
		return monitor_profile;
	}

	void update_sensitivity () {
		// TODO
	}

	void update_sensitivity_for_clipboard () {
		unowned var clipboard = get_clipboard ();
		unowned var formats = clipboard.get_formats ();
		var images = Gdk.ContentFormats.parse ("image/png");
		var can_open = formats.match (images);
		Util.enable_action (action_group, "open-clipboard", can_open);
	}

	void before_closing () {
		if (app.one_window () && get_realized ()) {
			browser.save_preferences (current_page == Page.BROWSER);
			viewer.save_preferences (current_page == Page.VIEWER);
		}
		viewer.release_resources ();
		browser.release_resources ();
	}

	public GenericArray<Gth.FileData> get_selected () {
		if (current_page == Page.VIEWER) {
			return viewer.get_selected ();
		}
		return browser.get_selected ();
	}

	public File get_current_file () {
		if (current_page == Page.VIEWER) {
			return viewer.current_file.file;
		}
		return null;
	}

	async void open_selected_files () {
		var local_job = new_job ("Choose application");
		try {
			var files = get_selected ();
			if (files.length == 0) {
				throw new IOError.FAILED (_("No file selected"));
			}
			var app_selector = new Gth.AppSelector ();
			var app_info = yield app_selector.select_app (this, files, local_job.cancellable);
			var uris = new List<string>();
			foreach (unowned var file_data in files) {
				uris.append (file_data.file.get_uri ());
			}
			var context = get_display ().get_app_launch_context ();
			context.set_timestamp (0);
			context.set_icon (app_info.get_icon ());
			app_info.launch_uris (uris, context);
		}
		catch (Error error) {
			show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	public async void open_clipboard () {
		var clipboard = get_clipboard ();
		var local_job = new_job ("Open clipboard");
		try {
			var texture = yield clipboard.read_texture_async (local_job.cancellable);
			var image = new Gth.Image.from_texture (texture);
			var timestamp = new GLib.DateTime.now_local ();
			var basename = _("Clipboard") + timestamp.format (" %Y-%m-%d %H-%M-%S.jpeg");
			var file = browser.folder_tree.current_folder.file.get_child (basename);
			var unsaved_file = new Gth.FileData (file);
			unsaved_file.info.set_display_name (basename);
			unsaved_file.info.set_edit_name (basename);
			unsaved_file.set_content_type ("image/jpeg");
			unsaved_file.set_is_modified (true);
			unsaved_file.info.set_attribute_boolean (PrivateAttribute.LOADED_IMAGE_FROM_CLIPBOARD, true);
			// Use this window if the viewer is ImageViewer and the file is
			// not modified.
			var window = new Gth.Window (application);
			window.viewer.open_unsaved_image.begin (unsaved_file, image);
			window.present ();
		}
		catch (Error error) {
			show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	void init_actions () {
		var action = new SimpleAction ("pop-page", null);
		action.activate.connect (() => {
			var next_page = Page.NONE;
			if (current_page == Page.VIEWER) {
				 next_page = Page.BROWSER;
			}
			if (next_page != Page.NONE) {
				set_page.begin (next_page);
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("toggle-fullscreen", null);
		action.activate.connect (() => {
			fullscreened = !fullscreened;
		});
		action_group.add_action (action);

		notify["fullscreened"].connect (() => {
			if (fullscreened) {
				viewer.after_fullscreen ();
			}
			else {
				viewer.after_unfullscreen ();
			}
		});

		action = new SimpleAction ("open-with", null);
		action.activate.connect ((_action, param) => {
			open_selected_files.begin ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("open-clipboard", null);
		action.activate.connect (() => open_clipboard.begin ());
		action_group.add_action (action);

		action = new SimpleAction ("cancel-job", VariantType.UINT64);
		action.activate.connect ((_action, param) => {
			var id = param.get_uint64 ();
			// TODO
		});
		action_group.add_action (action);
	}

	[GtkChild] public unowned Gtk.Stack stack;
	[GtkChild] public unowned Gth.Browser browser;
	[GtkChild] public unowned Gth.Viewer viewer;
	public unowned Gth.SidebarResizer active_resizer;

	Page current_page = Page.NONE;
}
