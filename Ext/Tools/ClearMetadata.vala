public class Gth.ClearMetadata : Gth.FileOperation {
	public ClearMetadata () {
		factory = app.io_factory;
	}

	public override async void exec (File file, Cancellable cancellable) throws Error {
		var job = new Job ();
		job.callback = exec.callback;
		job.file = file;
		job.cancellable = cancellable;
		factory.add_job (job);
		yield;
		if (job.error != null) {
			throw job.error;
		}
		app.monitor.file_changed (job.file);
	}

	class Job : Work.Job {
		public File file;
		public Cancellable cancellable;

		public override void run (uint8[] tmp_buffer) throws Error {
			// Delete the embedded metadata.
			var bytes = Files.load_file (file, cancellable);
			bytes = Exiv2.clear_metadata (bytes);
			Files.save_file (file, bytes, SaveFileFlags.DEFAULT, cancellable);

			// Delete the sidecar as well.
			var comment_file = Comment.get_comment_file (file);
			Files.delete_file (comment_file, cancellable);
		}
	}

	weak Work.Factory factory;
	Transform transform;
	TransformFlags flags;
}
