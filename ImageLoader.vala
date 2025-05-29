public class Gth.ImageLoader {
	public ImageLoader (uint workers) {
		jobs = new AsyncQueue<Job?>();
		threads = new Queue<Thread<void>>();
		ThreadFunc<void> worker_func = () => {
			while (true) {
				var job = jobs.pop ();
				if (job.operation == Operation.EXIT)
					break;
				job.run ();
			}
		};
		for (uint i = 0; i < workers; i++) {
			threads.push_head (new Thread<void> ("ImageLoader", worker_func));
		}
	}

	~ImageLoader() {
		// Exit the threads.
		for (var i = 0; i < threads.length; i++) {
			jobs.push (new Job.exit ());
		}
		while (threads.length > 0) {
			var thread = threads.pop_head ();
			thread.join ();
		}
	}

	Queue<Thread<void>> threads;

	public async Image? load_from_stream (InputStream stream, File? file, Cancellable cancellable, uint requested_size = 0) throws Error {
		var job = new Job ();
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

		public Job () {
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
				// TODO
				//unowned var icc_profile = image.get_icc_profile ();
				//if (icc_profile != null) {
				//	yield image.apply_icc_profile_async (app.get_color_manager (), icc_profile, cancellable);
				//}
			}
			catch (Error local_error) {
				error = local_error;
			}
			Idle.add ((owned) callback);
		}
	}

	AsyncQueue<Job?> jobs;
}

[CCode (has_target = false)]
public delegate Gth.Image Gth.ImageLoaderFunc (Bytes bytes, uint requested_size, Cancellable cancellable) throws Error;
