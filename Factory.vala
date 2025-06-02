public class Work.Factory {
	public uint n_workers;

	public Factory (uint _n_workers) {
		jobs = new AsyncQueue<Work.Job>();
		n_workers = _n_workers;
		workers = new Queue<Thread<void>>();
		ThreadFunc<void> worker_func = () => {
			while (true) {
				var job = jobs.pop ();
				if (job.operation == Job.Operation.EXIT)
					break;
				job.try_run ();
			}
		};
		for (uint i = 0; i < n_workers; i++) {
			workers.push_head (new Thread<void> ("Factory::Worker%u".printf (i), worker_func));
		}
	}

	~Factory() {
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

	AsyncQueue<Work.Job> jobs;
	Queue<Thread<void>> workers;
}


public class Work.Job {
	public enum Operation {
		RUN,
		EXIT
	}

	public Operation operation;
	public SourceFunc callback;
	public Error error;

	public Job (SourceFunc _callback) {
		operation = Operation.RUN;
		callback = _callback;
		error = null;
	}

	public Job.exit () {
		operation = Operation.EXIT;
		callback = null;
		error = null;
	}

	public void try_run () {
		try {
			run ();
		}
		catch (Error local_error) {
			error = local_error;
		}
		Idle.add ((owned) callback);
	}

	public virtual void run () throws Error {
		// void
	}
}

