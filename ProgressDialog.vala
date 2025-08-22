[GtkTemplate (ui = "/app/gthumb/gthumb/ui/progress-dialog.ui")]
public class Gth.ProgressDialog : Adw.Dialog {
	public ProgressDialog (Gth.Window _window) {
		window = _window;
		closed.connect (() => {
			presented = false;
		});
		jobs = new GenericList<Job>();
		job_list.bind_model (jobs.model, (obj) => {
			return new ProgressRow (this, obj as Job);
		});
	}

	public void set_queue (JobQueue _queue) {
		queue = _queue;
		foreach (unowned var job in jobs) {
			jobs.model.append (job);
		}
		after_changing_jobs ();
		queue.job_added.connect ((job) => add_job (job));
	}

	public void show_dialog () {
		cancel_show_dialog ();
		if (job_dialogs > 0) {
			return;
		}
		present (window);
		presented = true;
	}

	public void hide_dialog () {
		cancel_show_dialog ();
		if (presented) {
			close ();
		}
	}

	void cancel_show_dialog () {
		if (show_event != 0) {
			Source.remove (show_event);
			show_event = 0;
		}
	}

	void queue_show_dialog () {
		if (show_event == 0) {
			show_event = Util.after_timeout (SHOW_DELAY, () => show_dialog ());
		}
	}

	void add_job (Job job) {
		jobs.model.insert (0, job);
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
		cancel_all.visible = jobs.model.n_items > 3;
		job_dialogs_changed ();
	}

	uint job_dialogs = 0;

	public void job_dialogs_changed () {
		job_dialogs = 0;
		foreach (unowned var job in jobs) {
			job_dialogs += job.open_dialogs;
		}
		//stdout.printf ("> job_dialogs: %u\n", job_dialogs);
		if ((job_dialogs > 0) || (jobs.model.n_items == 0)) {
			hide_dialog ();
		}
		else {
			queue_show_dialog ();
		}
	}

	int count_foreground_jobs () {
		var count = 0;
		foreach (unowned var job in jobs) {
			if (job.foreground) {
				count++;
			}
		}
		return count;
	}

	~ProgressDialog() {
		cancel_show_dialog ();
	}

	weak Gth.Window window;
	JobQueue queue;
	GenericList<Job> jobs;
	[GtkChild] unowned Gtk.ListBox job_list;
	[GtkChild] unowned Gtk.Box cancel_all;
	bool presented = false;
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

		activatable = true;
		set_title (job.description);

		var button = new Gtk.Button.with_label (_("Cancel"));
		button.clicked.connect (() => job.cancel ());
		button.valign = Gtk.Align.CENTER;

		var progress = new Gtk.ProgressBar ();
		progress.pulse_step = 0.2;
		job.notify["progress"].connect ((obj, _prop) => {
			progress.fraction = (double) job.progress;
		});

		Timeout.add (250, () => {
			if (progress.fraction == 0.0) {
				progress.pulse ();
			}
			return Source.CONTINUE;
		});

		var box = new Gtk.Box (Gtk.Orientation.VERTICAL, 12);
		box.append (button);
		box.append (progress);
		box.margin_top = 12;
		box.margin_bottom = 12;
		add_suffix (box);

		var icon = new Gtk.Image.from_icon_name ("gth-script-symbolic");
		add_prefix (icon);
	}

	weak ProgressDialog dialog;
}
