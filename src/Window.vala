public class Gth.Window : Adw.ApplicationWindow {
	public Gth.JobQueue jobs;
	public MonitorProfile monitor_profile;
	public bool closing;
	public SimpleActionGroup action_group;

	public void set_busy (bool busy) {
		cursor = busy ? new Gdk.Cursor.from_name ("progress", null) : null;
	}

	public virtual void add_toast (Adw.Toast toast) {}

	public void show_message (string message) {
		add_toast (Util.new_literal_toast (message));
	}

	public void show_error (Error error) {
		if ((error is IOError.CANCELLED)
			|| (error is Gtk.DialogError.DISMISSED)
			|| (error is Gtk.DialogError.CANCELLED))
		{
			return;
		}
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

	public Gth.Job new_job (string description, JobFlags flags = JobFlags.DEFAULT, string? icon_name = null) {
		return jobs.new_job (description, flags, icon_name);
	}

	public virtual void init_actions () {
		var action = new SimpleAction ("job-queue", null);
		action.activate.connect ((_action, param) => {
			progress_dialog.show_dialog (true);
		});
		action_group.add_action (action);

		action = new SimpleAction ("fake-job", null);
		action.activate.connect ((_action, param) => {
			add_fake_job ();
			Util.after_timeout (1500, () => add_fake_job ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("cancel-all-jobs", null);
		action.activate.connect ((_action, param) => {
			cancel_jobs ();
		});
		action_group.add_action (action);
	}

	public virtual bool on_close () {
		if (jobs.has_foreground_jobs ()) {
			ensure_progress_dialog ();
			progress_dialog.show_dialog (true);
			return true;
		}
		closing = true;
		if (cancel_jobs ()) {
			return true;
		}
		before_closing ();
		return false;
	}

	public virtual void before_closing () {
		progress_dialog.force_close ();
	}

	public virtual void on_jobs_changed () {
		if (closing && (jobs.size () == 0)) {
			Util.next_tick (() => {
				before_closing ();
				close ();
			});
			return;
		}
		ensure_progress_dialog ();
	}

	void ensure_progress_dialog () {
		if (progress_dialog == null) {
			progress_dialog = new ProgressDialog (this);
			progress_dialog.set_queue (jobs);
		}
	}

	bool cancel_jobs () {
		if (jobs.size () == 0) {
			return false;
		}
		jobs.cancel_all ();
		return true;
	}

	void add_fake_job () {
		fake_job_id++;
		var local_job = new_job ("Fake Job %u".printf (fake_job_id),
			JobFlags.FOREGROUND,
			"applications-science-symbolic");
		local_job.subtitle = Strings.new_random (50);
		local_job.cancellable.cancelled.connect (() => {
			local_job.done ();
		});
		//local_job.progress = 0.3f;
	}

	construct {
		monitor_profile = new MonitorProfile (this);
		jobs = new Gth.JobQueue ();
		jobs.size_changed.connect (() => on_jobs_changed ());
		close_request.connect (() => on_close ());
		closing = false;
		action_group = new SimpleActionGroup ();
		insert_action_group ("win", action_group);
		init_actions ();
	}

	uint fake_job_id = 0;
	ProgressDialog progress_dialog = null;
}
