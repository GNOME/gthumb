public class Gth.ImageSaver {
	public weak Work.Factory factory;

	public ImageSaver (Work.Factory _factory) {
		factory = _factory;
	}

	public async void save_to_stream (OutputStream stream, string content_type, Image image, Cancellable cancellable) throws Error {
		var job = new Job (save_to_stream.callback);
		job.stream = stream;
		job.content_type = content_type;
		job.image = image;
		job.cancellable = cancellable;
		factory.add_job (job);
		yield;
		if (job.error != null) {
			throw job.error;
		}
	}

	public async void save_to_file (File file, string content_type, Image image, Cancellable cancellable) throws Error {
		var stream = yield file.replace_async (null, false, FileCreateFlags.NONE, Priority.DEFAULT, cancellable);
		yield save_to_stream (stream, content_type, image, cancellable);
	}

	class Job : Work.Job {
		public OutputStream stream;
		public string content_type;
		public Image image;
		public Cancellable cancellable;

		public Job (SourceFunc callback) {
			base (callback);
			image = null;
		}

		public override void run (uint8[] buffer) throws Error {
			var save_func = app.get_save_func (content_type);
			if (save_func == null) {
				throw new IOError.NOT_SUPPORTED (_("Cannot save this kind of files"));
			}
			var bytes = save_func (image, cancellable);
			if (bytes == null) {
				throw new IOError.FAILED ("Save failed");
			}
			stream.write_all (bytes.get_data (), null, cancellable);
		}
	}
}

[CCode (has_target = false)]
public delegate Bytes? Gth.SaveFunc (Gth.Image image, Cancellable cancellable) throws Error;
