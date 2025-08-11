public class Gth.ImageSaver {
	public weak Work.Factory factory;

	public ImageSaver (Work.Factory _factory) {
		factory = _factory;
	}

	public async void save_to_stream (Image image, OutputStream stream, string content_type, Cancellable cancellable) throws Error {
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

	public async void replace_file (Image image, string? etag, File file, string content_type, Cancellable cancellable) throws Error {
		var stream = yield file.replace_async (etag, false,
			FileCreateFlags.NONE, Priority.DEFAULT,
			cancellable);
		yield save_to_stream (image, stream, content_type, cancellable);
		stream.close ();
		image.set_attribute ("etag", stream.get_etag ());
	}

	public async void create_file (Image image, File file, string content_type, Cancellable cancellable) throws Error {
		var stream = yield file.create_async (FileCreateFlags.NONE,
			Priority.DEFAULT,
			cancellable);
		yield save_to_stream (image, stream, content_type, cancellable);
		stream.close ();
		image.set_attribute ("etag", stream.get_etag ());
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
			Gth.Option[] options = null;
			var preferences = app.get_saver_preferences (content_type);
			if (preferences != null) {
				options = preferences.get_options ();
			}
			// TODO
			//if (preferences.can_save_icc_profile ()) {
			//	image.apply_icc_profile (app.color_manager, new Gth.IccProfile.sRGB (), cancellable);
			//}
			var bytes = save_func (image, options, cancellable);
			if (bytes == null) {
				throw new IOError.FAILED ("Save failed");
			}
			stream.write_all (bytes.get_data (), null, cancellable);
		}
	}
}

[CCode (has_target = false)]
public delegate Bytes? Gth.SaveFunc (
	Gth.Image image,
	[CCode (array_length = false, array_null_terminated = true)] Gth.Option[]? options,
	Cancellable cancellable) throws Error;
