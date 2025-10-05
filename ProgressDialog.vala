[GtkTemplate (ui = "/app/gthumb/gthumb/ui/progress-dialog.ui")]
public class Gth.ProgressDialog : Adw.Dialog {
	public ProgressDialog (Gth.Window _window) {
		window = _window;
		closed.connect (() => {
			presented = false;
			presented_by_user = false;
		});
		jobs = new GenericList<Job>();
		job_list.bind_model (jobs.model, (obj) => {
			return new ProgressRow (this, obj as Job);
		});
	}

	public void set_queue (JobQueue _queue) {
		queue = _queue;
		foreach (unowned var job in queue.queue) {
			jobs.model.append (job);
		}
		after_changing_jobs ();
		queue.job_added.connect ((job) => add_job (job));
	}

	public void show_dialog (bool by_user = false) {
		cancel_show_dialog ();
		if (job_dialogs > 0) {
			return;
		}
		present (window);
		presented = true;
		presented_by_user = by_user;
	}

	public void hide_dialog () {
		cancel_show_dialog ();
		if (presented) {
			close ();
		}
	}

	void queue_show_dialog () {
		if (show_event == 0) {
			show_event = Util.after_timeout (SHOW_DELAY, () => show_dialog ());
		}
	}

	void cancel_show_dialog () {
		if (show_event != 0) {
			Source.remove (show_event);
			show_event = 0;
		}
	}

	void add_job (Job job) {
		jobs.model.append (job);
		after_changing_jobs ();
	}

	public void remove_row (ProgressRow row) {
		var pos = jobs.find (row.job);
		if (pos == uint.MAX) {
			return;
		}
		jobs.model.remove (pos);
		after_changing_jobs ();
	}

	void after_changing_jobs () {
		cancel_all.visible = jobs.model.n_items > 2;
		job_dialogs_changed ();
	}

	uint foreground_jobs = 0;
	uint job_dialogs = 0;

	public void job_dialogs_changed () {
		var prev_foreground_jobs = foreground_jobs;
		foreground_jobs = 0;
		job_dialogs = 0;
		foreach (unowned var job in jobs) {
			job_dialogs += job.open_dialogs;
			if (job.foreground) {
				foreground_jobs += 1;
			}
		}
		if ((job_dialogs > 0)
			|| (jobs.model.n_items == 0)
			|| (!presented_by_user && (foreground_jobs == 0)))
		{
			hide_dialog ();
		}
		else if (foreground_jobs > prev_foreground_jobs) {
			queue_show_dialog ();
		}
	}

	~ProgressDialog() {
		cancel_show_dialog ();
	}

	weak Gth.Window window;
	JobQueue queue;
	GenericList<Job> jobs;
	[GtkChild] unowned Gtk.ListBox job_list;
	[GtkChild] unowned Gtk.Button cancel_all;
	bool presented = false;
	bool presented_by_user = false;
	uint show_event = 0;

	const uint SHOW_DELAY = 500;
}


public class Gth.ProgressRow : Adw.ActionRow {
	public Job job;

	public ProgressRow (ProgressDialog _dialog, Job _job) {
		dialog = _dialog;
		job = _job;
		job.notify["terminated"].connect ((obj, _prop) => {
			dialog.remove_row (this);
		});
		job.notify["open-dialogs"].connect ((obj, _prop) => {
			dialog.job_dialogs_changed ();
		});
		job.notify["subtitle"].connect ((obj, _prop) => {
			subtitle = job.subtitle;
		});

		activatable = true;
		use_markup = false;
		title_lines = 3;
		subtitle_lines = 1;
		title = job.title;
		subtitle = job.subtitle;

		var button = new Gtk.Button.with_label (_("Cancel"));
		button.clicked.connect (() => job.cancel ());
		button.valign = Gtk.Align.CENTER;

		var progress = new Gtk.ProgressBar ();
		progress.pulse_step = 0.2;
		job.notify["progress"].connect ((obj, _prop) => {
			progress.fraction = (double) job.progress;
		});

		Timeout.add (250, () => {
			if (progress.fraction > 0.0) {
				return Source.REMOVE;
			}
			progress.pulse ();
			return Source.CONTINUE;
		});

		var box = new Gtk.Box (Gtk.Orientation.VERTICAL, 12);
		box.append (button);
		box.append (progress);
		box.margin_top = 12;
		box.margin_bottom = 12;
		add_suffix (box);

		var icon = new Gtk.Image.from_icon_name (job.icon_name ?? "gth-script-symbolic");
		icon.icon_size = Gtk.IconSize.LARGE;
		add_prefix (icon);
	}

	weak ProgressDialog dialog;
}
