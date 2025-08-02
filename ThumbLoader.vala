public class Gth.ThumbLoader {
	public weak Work.Factory factory;

	public ThumbLoader (Work.Factory _factory) {
		factory = _factory;
	}

	public async Image? load_if_valid (File thumb_file, FileData file_data, Cancellable cancellable) throws Error {
		var job = new Job (load_if_valid.callback);
		job.thumb_file = thumb_file;
		job.file_data = file_data;
		job.cancellable = cancellable;
		factory.add_job (job);
		yield;
		if (job.error != null) {
			throw job.error;
		}
		return job.image;
	}

	class Job : Work.Job {
		public File thumb_file;
		public FileData file_data;
		public Cancellable cancellable;
		public Image image;

		public Job (SourceFunc callback) {
			base (callback);
			image = null;
		}

		public override void run (uint8[] buffer) throws Error {
			var stream = thumb_file.read (cancellable);
			var bytes = Files.read_all_with_buffer (stream, cancellable, buffer);
			if (!Thumbnailer.valid_thumbnail_for_file (bytes, file_data, cancellable)) {
				throw new IOError.FAILED ("Invalid thumbnail");
			}
			image = load_png (bytes, 0, cancellable);
		}
	}
}
