public class Gth.Job : Object {
	public enum State {
		RUNNING,
		CANCELLED,
		COMPLETED,
	}

	public Cancellable cancellable;
	public uint64 id { get; default = Job.next_id (); }
	public float progress { get; set; default = 0.0f; }
	public string description { get; set; default = ""; }
	public State state { get; set; default = State.RUNNING; }
	public bool terminated { get; private set; default = false; }
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
		//	description
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
		terminated = true;
		if (toast != null) {
			toast.dismiss ();
		}
		//stdout.printf ("  JOB TERMINATED [%p] [%s] (state: %s) (seconds: %d): %s\n",
		//	this,
		//	time_terminated.format ("%H:%M:%S"),
		//	state.to_string (),
		//	seconds (),
		//	description
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
}

public class Gth.JobQueue : Object {
	public Queue<Gth.Job> queue;

	public JobQueue () {
		queue = new Queue<Gth.Job>();
	}

	public Gth.Job new_job (string description, JobFlags flags) {
		var job = new Gth.Job ();
		job.description = description;
		job.foreground = JobFlags.FOREGROUND in flags;
		//stdout.printf ("  NEW %sJOB [%p] [%s]: %s\n",
		//	(job.foreground ? "FOREGROUND " : ""),
		//	job,
		//	job.time_started.format ("%H:%M:%S"),
		//	job.description);
		add_job (job);
		return job;
	}

	public void remove_job (Gth.Job job) {
		if (queue.remove (job)) {
			size_changed ();
		}
	}

	public void add_job (Gth.Job job) {
		if (queue.find (job) != null)
			return;
		job.notify["terminated"].connect ((obj, _prop) => remove_job (obj as Gth.Job));
		queue.push_tail (job);
		size_changed ();
	}

	public inline uint size () {
		return queue.length;
	}

	public unowned Gth.Job first () {
		return queue.peek_head ();
	}

	public void cancel_all () {
		var local_queue = queue.copy ();
		Job job = null;
		while ((job = local_queue.pop_head ()) != null) {
			job.cancel ();
		}
	}

	public Job find_by_id (uint64 id) {
		unowned var item = queue.search<uint64?> (id, (job, id) => {
			return (job.id == id) ? 0 : 1;
		});
		return (item != null) ? item.data : null;
	}

	public signal void size_changed ();
}


[Flags]
public enum Gth.JobFlags {
	DEFAULT,
	FOREGROUND,
}
