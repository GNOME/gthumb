public class Gth.Job : Object {
	public enum State {
		RUNNING,
		CANCELLED,
		COMPLETED,
	}

	public Cancellable cancellable;
	public float progress { get; set; default = 0.0f; }
	public string description { get; set; default = ""; }
	public string status { get; set; default = ""; }
	public State state { get; set; default = State.RUNNING; }
	public bool terminated { get; private set; default = false; }
	public GLib.DateTime time_started;
	public GLib.DateTime time_terminated;
	public bool background;
	public string? message;
	public Error? error;

	public Job () {
		cancellable = new Cancellable ();
		cancellable.cancelled.connect (() => {
			state = State.CANCELLED;
		});
		time_started = new GLib.DateTime.now ();
		background = false;
		message = null;
		error = null;
	}

	public bool cancel () {
		if (state != State.RUNNING)
			return false;
		/*var time_cancelled = new GLib.DateTime.now ();
		stdout.printf ("  JOB CANCELLED [%p] [%s]: %s\n",
			this,
			time_cancelled.format ("%H:%M:%S"),
			description
		);*/
		state = State.CANCELLED;
		cancellable.cancel ();
		return true;
	}

	public void done () {
		if (terminated)
			return;
		time_terminated = new GLib.DateTime.now ();
		if (state == State.RUNNING) {
			state = State.COMPLETED;
			progress = 1.0f;
		}
		terminated = true;
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

	public Gth.Job new_job (string description, string? status = null, bool background = false) {
		var job = new Gth.Job ();
		job.description = description;
		job.status = status;
		job.background = background;
		//stdout.printf ("  NEW JOB [%p] [%s]: %s%s\n",
		//	job,
		//	job.time_started.format ("%H:%M:%S"),
		//	job.description,
		//	job.background ? " (background)" : "");
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

	public signal void size_changed ();
}
