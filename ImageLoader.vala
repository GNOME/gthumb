public class Gth.ImageLoader {
	public weak Work.Factory factory;

	public ImageLoader (Work.Factory _factory) {
		factory = _factory;
	}

	public async Image? load_stream (InputStream stream, File? file, Cancellable cancellable, uint requested_size = 0) throws Error {
		var job = new Job ();
		job.callback = load_stream.callback;
		job.stream = stream;
		job.file = file;
		job.cancellable = cancellable;
		job.requested_size = requested_size;
		factory.add_job (job);
		yield;
		if (job.error != null) {
			throw job.error;
		}
		return job.image;
	}

	public async Image? load_file (File file, Cancellable cancellable, uint requested_size = 0) throws Error {
		var stream = yield file.read_async (Priority.DEFAULT, cancellable);
		var image = yield load_stream (stream, file, cancellable, requested_size);
		var info = yield stream.query_info_async (FileAttribute.ETAG_VALUE, Priority.DEFAULT, cancellable);
		if (info.has_attribute (FileAttribute.ETAG_VALUE)) {
			image.set_attribute ("etag", info.get_attribute_string (FileAttribute.ETAG_VALUE));
		}
		return image;
	}

	class Job : Work.Job {
		public InputStream stream;
		public File? file;
		public Cancellable cancellable;
		public uint requested_size;
		public Image image;

		public Job () {
			image = null;
		}

		public override void run (uint8[] buffer) throws Error {
			var content_type = Util.guess_content_type_from_stream (stream, file, cancellable);
			if (content_type == null) {
				throw new IOError.FAILED (_("Unknown file type"));
			}

			var load_func = app.get_load_func (content_type);
			if (load_func != null) {
				if (stream is Seekable) {
					var seekable = stream as Seekable;
					seekable.seek (0, SeekType.SET, cancellable);
				}
				var bytes = Files.read_all_with_buffer (stream, cancellable, buffer);
				image = load_func (bytes, requested_size, cancellable);
			}
			else {
				var load_file_func = app.get_load_file_func (content_type);
				if ((file != null) && (load_file_func != null)) {
					image = load_file_func (file, requested_size, cancellable);
				}
				else {
					throw new IOError.NOT_SUPPORTED (_("No suitable loader available for this file type"));
				}
			}

			image.set_attribute ("content-type", content_type);
		}
	}
}

[CCode (has_target = false)]
public delegate Gth.Image? Gth.LoadFunc (Bytes bytes, uint requested_size, Cancellable cancellable) throws Error;

[CCode (has_target = false)]
public delegate Gth.Image? Gth.LoadFileFunc (File file, uint requested_size, Cancellable cancellable) throws Error;
