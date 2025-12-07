public class Gth.Job : Object {
	public enum State {
		RUNNING,
		CANCELLED,
		COMPLETED,
	}

	public Cancellable cancellable;
	public uint64 id { get; default = Job.next_id (); }
	public float progress { get; set; default = 0.0f; }
	public string title { get; set; default = ""; }
	public string subtitle { get; set; default = null; }
	public string icon_name { get; set; default = null; }
	public State state { get; set; default = State.RUNNING; }
	public bool terminated { get; private set; default = false; }
	public uint open_dialogs { get; private set; default = 0; }
	public GLib.DateTime time_started;
	public GLib.DateTime time_terminated;
	public bool foreground;
	public string? message;
	public Error? error;
	public Adw.Toast toast;

	static Mutex mutex = Mutex ();
	static uint64 last_id = 0;

	public static uint64 next_id () {
		mutex.lock ();
		last_id++;
		mutex.unlock ();
		return last_id;
	}

	public Job () {
		cancellable = new Cancellable ();
		cancellable.cancelled.connect (() => {
			state = State.CANCELLED;
		});
		time_started = new GLib.DateTime.now ();
		foreground = false;
		message = null;
		error = null;
		toast = null;
	}

	public bool cancel () {
		if (state != State.RUNNING) {
			return false;
		}
		//var time_cancelled = new GLib.DateTime.now ();
		//stdout.printf ("  JOB CANCELLED [%p] [%s]: %s\n",
		//	this,
		//	time_cancelled.format ("%H:%M:%S"),
		//	title
		//);
		state = State.CANCELLED;
		cancellable.cancel ();
		return true;
	}

	public void done () {
		if (terminated) {
			return;
		}
		time_terminated = new GLib.DateTime.now ();
		if (state == State.RUNNING) {
			state = State.COMPLETED;
			progress = 1.0f;
		}
		open_dialogs = 0;
		terminated = true;
		if (toast != null) {
			toast.dismiss ();
		}
		//stdout.printf ("  JOB TERMINATED [%p] [%s] (state: %s) (seconds: %d): %s\n",
		//	this,
		//	time_terminated.format ("%H:%M:%S"),
		//	state.to_string (),
		//	seconds (),
		//	title
		//);
	}

	public void set_cancelled () {
		if (state == State.RUNNING) {
			state = State.CANCELLED;
		}
	}

	public bool is_cancelled () {
		return state == State.CANCELLED;
	}

	public bool is_running () {
		return state == State.RUNNING;
	}

	public int seconds () {
		return (int) (microseconds () / 1000000);
	}

	public int milliseconds () {
		return (int) (microseconds () / 1000);
	}

	public TimeSpan microseconds () {
		if (terminated) {
			return time_terminated.difference (time_started);
		}
		else {
			var now = new GLib.DateTime.now ();
			return now.difference (time_started);
		}
	}

	public void opens_dialog () {
		open_dialogs += 1;
	}

	public void dialog_closed () {
		if (open_dialogs > 0) {
			open_dialogs -= 1;
		}
	}
}

public class Gth.JobQueue : Object {
	public GenericArray<Gth.Job> queue;

	public signal void size_changed ();
	public signal void job_added (Job job);

	public JobQueue () {
		queue = new GenericArray<Gth.Job>();
	}

	public Gth.Job new_job (string title, JobFlags flags = JobFlags.DEFAULT, string? icon_name = null) {
		var job = new Gth.Job ();
		job.title = title;
		job.foreground = JobFlags.FOREGROUND in flags;
		job.icon_name = icon_name;
		//stdout.printf ("  NEW %sJOB [%p] [%s]: %s\n",
		//	(job.foreground ? "FOREGROUND " : ""),
		//	job,
		//	job.time_started.format ("%H:%M:%S"),
		//	job.title);
		add_job (job);
		return job;
	}

	public void add_job (Gth.Job job) {
		if (queue.find (job))
			return;
		job.notify["terminated"].connect ((obj, _prop) => remove_job (obj as Gth.Job));
		queue.add (job);
		job_added (job);
		size_changed ();
	}

	public inline uint size () {
		return queue.length;
	}

	public unowned Gth.Job first () {
		return queue[0];
	}

	public bool has_foreground_jobs () {
		foreach (unowned var job in queue) {
			if (job.foreground) {
				return true;
			}
		}
		return false;
	}

	public void cancel_all () {
		var local_queue = new GenericArray<Gth.Job>();
		foreach (unowned var job in queue) {
			local_queue.add (job);
		}
		while (local_queue.length > 0) {
			var idx = local_queue.length - 1;
			var job = local_queue[idx];
			job.cancel ();
			local_queue.remove_index (idx);
		}
	}

	public Job find_by_id (uint64 id) {
		uint index = 0;
		var found = queue.find_custom<uint64?> (id, (job, id) => {
			return (job.id == id);
		}, out index);
		return found ? queue[index] : null;
	}

	void remove_job (Gth.Job job) {
		if (queue.remove (job)) {
			size_changed ();
		}
	}
}


[Flags]
public enum Gth.JobFlags {
	DEFAULT,
	FOREGROUND,
}
