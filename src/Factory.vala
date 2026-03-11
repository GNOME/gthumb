public class Work.Factory {
	public uint n_workers;

	public Factory (uint _n_workers) {
		jobs = new AsyncQueue<Work.Job>();
		n_workers = _n_workers;
		workers = new Queue<Thread<void>>();
		for (uint i = 0; i < n_workers; i++) {
			ThreadFunc<void> worker_func = () => {
				var buffer = new uint8[BUFFER_SIZE];
				while (true) {
					var job = jobs.pop ();
					if (job.action == Job.Action.EXIT) {
						break;
					}
					job.try_run (buffer);
				}
			};
			workers.push_head (new Thread<void> ("Factory::Worker%u".printf (i), (owned) worker_func));
		}
	}

	public void release_resources () {
		// Exit the threads.
		for (var i = 0; i < workers.length; i++) {
			jobs.push (new Work.Job.exit ());
		}
		while (workers.length > 0) {
			var thread = workers.pop_head ();
			thread.join ();
		}
	}

	public void add_job (Work.Job job) {
		jobs.push (job);
	}

	~Factory() {
		release_resources ();
	}

	AsyncQueue<Work.Job> jobs;
	Queue<Thread<void>> workers;

	const int BUFFER_SIZE = 256 * 1024;
}


public class Work.Job {
	public enum Action {
		RUN,
		EXIT
	}

	public Action action;
	public SourceFunc callback;
	public Error error;

	public Job () {
		action = Action.RUN;
		error = null;
	}

	public Job.exit () {
		action = Action.EXIT;
		callback = null;
		error = null;
	}

	public void try_run (uint8[] buffer) {
		try {
			run (buffer);
		}
		catch (Error local_error) {
			error = local_error;
		}
		Idle.add ((owned) callback);
	}

	public virtual void run (uint8[] buffer) throws Error {
		// void
	}
}
