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
	}

	public Window (Gtk.Application _app, File location, File? file_to_select) {
		Object (application: app, title: "Thumbnails");
		browser.window = this;
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

	public void copy_text (string text) {
		var clipboard = get_clipboard ();
		clipboard.set_text (text);
		show_message ("Copied to Clipboard");
	}

	public void set_page (Page page) {
		if (page == current_page)
			return;
		current_page = page;
		/*switch (current_page) {
		case Page.BROWSER:
			main_stack.set_visible_child (browser_page);
			break;
		case Page.VIEWER:
			main_stack.set_visible_child (viewer_page);
			break;
		}*/
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

	[GtkChild] public unowned Gth.Browser browser;

	Page current_page = Page.NONE;
}
