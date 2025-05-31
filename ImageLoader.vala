public class Gth.ImageLoader {
	public uint n_workers;

	public ImageLoader (uint _n_workers) {
		jobs = new AsyncQueue<Job?>();
		n_workers = _n_workers;
		workers = new Queue<Thread<void>>();
		ThreadFunc<void> worker_func = () => {
			while (true) {
				var job = jobs.pop ();
				if (job.operation == Operation.EXIT)
					break;
				job.run ();
			}
		};
		for (uint i = 0; i < n_workers; i++) {
			workers.push_head (new Thread<void> ("ImageLoader::Worker%u".printf (i), worker_func));
		}
	}

	~ImageLoader() {
		// Exit the threads.
		for (var i = 0; i < workers.length; i++) {
			jobs.push (new Job.exit ());
		}
		while (workers.length > 0) {
			var thread = workers.pop_head ();
			thread.join ();
		}
	}

	public async Image? load_from_stream (InputStream stream, File? file, Cancellable cancellable, uint requested_size = 0) throws Error {
		var job = new Job.load ();
		job.stream = stream;
		job.file = file;
		job.cancellable = cancellable;
		job.callback = load_from_stream.callback;
		job.requested_size = requested_size;
		jobs.push (job);
		yield; // Wait for the job to finish.
		if (job.error != null) {
			throw job.error;
		}
		return job.image;
	}

	public async Image? load_from_file (File file, Cancellable cancellable, uint requested_size = 0) throws Error {
		var stream = yield file.read_async (Priority.DEFAULT, cancellable);
		return yield load_from_stream (stream, file, cancellable, requested_size);
	}

	enum Operation {
		LOAD,
		EXIT
	}

	class Job {
		public InputStream stream;
		public File? file;
		public Cancellable cancellable;
		public SourceFunc callback;
		public Image image = null;
		public Error error = null;
		public Operation operation;
		public uint requested_size;

		public Job.load () {
			operation = Operation.LOAD;
		}

		public Job.exit () {
			operation = Operation.EXIT;
		}

		public void run () {
			try {
				var content_type = Util.guess_content_type_from_stream (stream, file, cancellable);
				if (content_type == null) {
					throw new IOError.FAILED (_("Unknown file type"));
				}

				var loader_func = app.get_image_loader (content_type);
				if (loader_func == null) {
					throw new IOError.NOT_SUPPORTED (_("No suitable loader available for this file type"));
				}

				var bytes = Files.read_all (stream, cancellable);
				image = loader_func (bytes, requested_size, cancellable);

				unowned var icc_profile = image.get_icc_profile ();
				if (icc_profile != null) {
					image.apply_icc_profile (app.color_manager, icc_profile, cancellable);
				}
			}
			catch (Error local_error) {
				error = local_error;
			}
			Idle.add ((owned) callback);
		}
	}

	AsyncQueue<Job?> jobs;
	Queue<Thread<void>> workers;
}

[CCode (has_target = false)]
public delegate Gth.Image Gth.ImageLoaderFunc (Bytes bytes, uint requested_size, Cancellable cancellable) throws Error;
