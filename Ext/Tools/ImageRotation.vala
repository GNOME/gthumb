public class Gth.ImageRotation : Gth.FileOperation {
	public ImageRotation (Transform _transform, TransformFlags _flags = TransformFlags.DEFAULT) {
		transform = _transform;
		flags = _flags;
		factory = app.io_factory;
	}

	public override async void exec (File file, Cancellable cancellable) throws Error {
		var job = new Job ();
		job.callback = exec.callback;
		job.file = file;
		job.transform = transform;
		job.flags = flags;
		job.cancellable = cancellable;
		factory.add_job (job);
		yield;
		if (job.error != null) {
			throw job.error;
		}
	}

	class Job : Work.Job {
		public File file;
		public Transform transform;
		public TransformFlags flags;
		public Cancellable cancellable;

		public override void run (uint8[] tmp_buffer) throws Error {
			// Read the file

			var input_stream = file.read (cancellable);
			var content_type = Util.guess_content_type_from_stream (input_stream, file, cancellable);
			if (content_type == null) {
				throw new IOError.FAILED (_("Unknown file type"));
			}

			if (input_stream is Seekable) {
				var seekable = input_stream as Seekable;
				seekable.seek (0, SeekType.SET, cancellable);
			}
			var bytes = Files.read_all_with_buffer (input_stream, cancellable, tmp_buffer);
			input_stream.close (cancellable);

			// Read the exif orientation

			var info = new FileInfo ();
			Exiv2.read_metadata_from_buffer (bytes, info, false);

			// Change orientation

			Metadata orientation_tag = null;
			if (info.has_attribute ("Exif::Image::Orientation")) {
				orientation_tag = info.get_attribute_object ("Exif::Image::Orientation") as Metadata;
			}

			Transform orientation = Transform.NONE;
			if (TransformFlags.RESET in flags) {
				orientation = transform;
			}
			else {
				if ((orientation_tag != null) && (orientation_tag.raw != null)) {
					if (!int.try_parse (orientation_tag.raw, out orientation, null, 10)) {
						throw new IOError.FAILED ("Invalid embedded orientation: %s".printf (orientation_tag.raw));
					}
					orientation = orientation.apply_transformation (transform);
				}
			}

			var raw_orientation = "%d".printf (orientation);
			if (orientation_tag == null) {
				orientation_tag = new Metadata.typed ("Short", raw_orientation);
				info.set_attribute_object ("Exif::Image::Orientation", orientation_tag);
			}
			else {
				orientation_tag.set ("raw", raw_orientation);
			}

			var change_orientation_tag = !(TransformFlags.CHANGE_IMAGE in flags) &&
				((content_type == "image/jpeg")
				 || (content_type == "image/tiff")
				 || (content_type == "image/webp"));

			if (change_orientation_tag) {
				Exiv2.update_dimensions (info, transform);
				bytes = Exiv2.write_metadata_to_buffer (bytes, info);
			}
			else if ((transform != Transform.NONE) || (TransformFlags.ALWAYS_SAVE in flags)) {
				// Load the image

				var load_func = app.get_load_func (content_type);
				if (load_func == null) {
					throw new IOError.NOT_SUPPORTED (_("No suitable loader available for this file type"));
				}
				var image = load_func (bytes, 0, cancellable);

				// Transform the image

				if (transform != Transform.NONE) {
					image = image.apply_transform (transform, cancellable);
					if (image == null) {
						throw new IOError.CANCELLED ("Cancelled");
					}
				}

				// Save the image to buffer

				var save_func = app.get_save_func (content_type);
				if (save_func == null) {
					throw new IOError.NOT_SUPPORTED (_("Cannot save this kind of files"));
				}

				Gth.Option[] options = null;
				var preferences = app.get_saver_preferences (content_type);
				if (preferences != null) {
					options = preferences.get_options ();
				}

				bytes = save_func (image, options, cancellable);
				if (bytes == null) {
					throw new IOError.FAILED ("Save failed");
				}

				// Save the metadata to buffer

				if (Exiv2.can_write_metadata (content_type)) {
					bytes = Exiv2.write_metadata_to_buffer (bytes, info, image);
				}
			}

			// Save the buffer to file
			Files.save_file (file, bytes, SaveFileFlags.DEFAULT, cancellable);
			app.monitor.file_changed (file);
		}
	}

	weak Work.Factory factory;
	Transform transform;
	TransformFlags flags;
}
