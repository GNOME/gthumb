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

	public void show_message (string message) {
		if (current_page == Page.BROWSER) {
			browser.show_message (message);
		}
	}

	public void show_error (Error error) {
		if (error is IOError.CANCELLED)
			return;
		show_message (error.message);
	}

	public void copy_text_to_clipboard (string text) {
		var clipboard = get_clipboard ();
		clipboard.set_text (text);
		show_message ("Copied to Clipboard");
	}

	public void set_page (Page page) {
		if (page == current_page)
			return;
		current_page = page;
		switch (current_page) {
		case Page.BROWSER:
			viewer.before_close_page ();
			stack.set_visible_child (browser);
			break;
		case Page.VIEWER:
			stack.set_visible_child (viewer);
			viewer.after_show_page ();
			break;
		}
		update_title ();
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

	void update_title () {
		// TODO
	}

	void update_sensitivity () {
		// TODO
	}

	void before_closing () {
		browser.save_preferences ();
	}

	void init_actions () {
		var action = new SimpleAction ("about", null);
		action.activate.connect (() => {
			const string[] developers = {
				"Paolo Bacchilega <paobac@src.gnome.org>",
			};
			Adw.show_about_dialog (this,
				"application-name", "Thumbnails",
				"application-icon", "app.gthumb.gthumb",
				"version", Config.VERSION,
				"license-type", Gtk.License.GPL_2_0,
				"translator-credits", _("translator-credits"),
				"website", "https://gitlab.gnome.org/GNOME/gthumb/",
				"issue-url", "https://gitlab.gnome.org/GNOME/gthumb/-/issues",
				"developers", developers
			);
		});
		action_group.add_action (action);

		action = new SimpleAction ("pop-page", null);
		action.activate.connect (() => {
			if (current_page == Page.VIEWER) {
				set_page (Page.BROWSER);
			}
		});
		action_group.add_action (action);
	}

	[GtkChild] public unowned Gtk.Stack stack;
	[GtkChild] public unowned Gth.Browser browser;
	[GtkChild] public unowned Gth.Viewer viewer;
	public unowned Gth.SidebarResizer active_resizer;

	Page current_page = Page.NONE;
}
