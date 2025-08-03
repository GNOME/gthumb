[GtkTemplate (ui = "/app/gthumb/gthumb/ui/window.ui")]
public class Gth.Window : Adw.ApplicationWindow {
	public Gth.JobQueue jobs;
	public bool closing;
	public SimpleActionGroup action_group;

	public enum Page {
		NONE = 0,
		BROWSER,
		VIEWER,
	}

	construct {
		jobs = new Gth.JobQueue ();
		jobs.size_changed.connect (() => {
			if (closing && (jobs.size () == 0)) {
				before_closing ();
				close ();
			}
		});
		closing = false;
		action_group = new SimpleActionGroup ();
		insert_action_group ("win", action_group);

		close_request.connect (() => {
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

		init_actions ();
	}

	public Window (Gtk.Application _app, File location, File? file_to_select) {
		Object (application: app, title: "Thumbnails");
		browser.window = this;
		viewer.window = this;
		set_page (Page.BROWSER);
		Util.next_tick (() => {
			browser.first_load (location, file_to_select);
		});
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
		add_toast (new Adw.Toast (message));
	}

	public void show_error (Error error) {
		if (error is IOError.CANCELLED)
			return;
		show_message (error.message);
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
		show_message ("Copied to Clipboard");
	}

	public void set_page (Page page) {
		if (page == current_page)
			return;
		var previuos_page = current_page;
		current_page = page;
		switch (current_page) {
		case Page.BROWSER:
			if (previuos_page == Page.VIEWER) {
				if (viewer.main_view.show_sidebar) {
					browser.content_view.max_sidebar_width = viewer.main_view.max_sidebar_width;
				}
				viewer.before_close_page ();
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
			browser.update_title ();
			break;
		case Page.VIEWER:
			if (previuos_page == Page.BROWSER) {
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

	public Gth.Job new_job (string description, string? status = null) {
		var job = app.new_job (description, status);
		add_job (job);
		return job;
	}

	public Gth.Job new_background_job (string description, string? status = null) {
		var job = app.new_job (description, status, true);
		add_job (job);
		return job;
	}

	public void add_job (Gth.Job job) {
		jobs.add_job (job);
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

	void before_closing () {
		if (!app.one_window () || !get_realized ()) {
			return;
		}
		browser.save_preferences (current_page == Page.BROWSER);
		viewer.save_preferences (current_page == Page.VIEWER);
	}

	void init_actions () {
		var action = new SimpleAction ("pop-page", null);
		action.activate.connect (() => {
			if (current_page == Page.VIEWER) {
				set_page (Page.BROWSER);
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("fullscreen", null);
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
	}

	[GtkChild] public unowned Gtk.Stack stack;
	[GtkChild] public unowned Gth.Browser browser;
	[GtkChild] public unowned Gth.Viewer viewer;
	public unowned Gth.SidebarResizer active_resizer;

	Page current_page = Page.NONE;
}
