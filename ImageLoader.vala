public class Gth.ImageLoader {
	public weak Work.Factory factory;

	public ImageLoader (Work.Factory _factory) {
		factory = _factory;
	}

	const string REQUIRED_ATTRIBUTES = STANDARD_ATTRIBUTES_WITH_FAST_CONTENT_TYPE + "," +
			 FileAttribute.ETAG_VALUE;

	public async Image? load_file (Gth.MonitorProfile? monitor_profile, File file,
		LoadFlags flags, Cancellable cancellable,
		uint requested_size = 0) throws Error
	{
		var info = yield file.query_info_async (REQUIRED_ATTRIBUTES,
			FileQueryInfoFlags.NONE, Priority.DEFAULT,
			cancellable);
		var stream = yield file.read_async (Priority.DEFAULT, cancellable);
		var image = yield load_stream (monitor_profile, stream, file, info, flags,
			cancellable, requested_size);
		return image;
	}

	async Image? load_stream (Gth.MonitorProfile? monitor_profile, InputStream stream, File? file,
		FileInfo info, LoadFlags flags, Cancellable cancellable,
		uint requested_size = 0) throws Error
	{
		var job = new Job ();
		job.callback = load_stream.callback;
		job.stream = stream;
		job.file = file;
		job.info = info;
		job.flags = flags;
		job.cancellable = cancellable;
		job.requested_size = requested_size;
		factory.add_job (job);
		yield;
		if (job.error != null) {
			throw job.error;
		}
		if (monitor_profile != null) {
			yield monitor_profile.apply_color_profile (job.image, info, cancellable, !(LoadFlags.NO_ICC_PROFILE in flags));
		}
		return job.image;
	}

	class Job : Work.Job {
		public InputStream stream;
		public File? file;
		public FileInfo info;
		public LoadFlags flags;
		public Cancellable cancellable;
		public uint requested_size;
		public Image image;

		public Job () {
			image = null;
		}

		public override void run (uint8[] tmp_buffer) throws Error {
			var content_type = Util.guess_content_type_from_stream (stream, file, cancellable);
			if (content_type == null) {
				throw new IOError.FAILED (_("Unknown file type"));
			}

			var seekable = stream as Seekable;

			if (LoadFlags.NO_BIG_IMAGES in flags) {
				int width, height;
				seekable.seek (0, SeekType.SET, cancellable);
				if (!load_image_info_from_stream (stream, out width, out height, cancellable)) {
					throw new IOError.FAILED (_("Unknown file type"));
				}
				if (width * height >= MAX_SIZE_FOR_PRELOADER) {
					throw new IOError.FAILED ("Ignoring big image: %d x %d".printf (width, height));
				}
			}

			var load_metadata = !(LoadFlags.NO_METADATA in flags);
			Bytes bytes = null;

			var load_func = app.get_load_func (content_type);
			if (load_func != null) {
				seekable.seek (0, SeekType.SET, cancellable);
				bytes = Files.read_all_with_buffer (stream, cancellable, tmp_buffer);
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

			info.set_attribute_string (FileAttribute.STANDARD_CONTENT_TYPE, content_type);
			var frames = image.get_frames ();
			if (frames > 1) {
				var metadata = new Metadata.for_string ("%u".printf (frames));
				info.set_attribute_object ("Animation::Frames", metadata);
			}

			if (load_metadata) {
				foreach (unowned var provider in app.metadata_providers) {
					if (provider.can_read (file, info)) {
						provider.read (file, bytes, info, cancellable);
					}
				}
			}

			image.info = info;
		}
	}
}

[CCode (has_target = false)]
public delegate Gth.Image? Gth.LoadFunc (Bytes bytes, uint requested_size, Cancellable cancellable) throws Error;

[CCode (has_target = false)]
public delegate Gth.Image? Gth.LoadFileFunc (File file, uint requested_size, Cancellable cancellable) throws Error;

[Flags]
public enum Gth.LoadFlags {
	DEFAULT = 0,
	NO_METADATA,
	NO_ICC_PROFILE,
	NO_BIG_IMAGES,
}
