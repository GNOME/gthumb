public class Gth.ImageSaver {
	public weak Work.Factory factory;

	public ImageSaver (Work.Factory _factory) {
		factory = _factory;
	}

	public async void replace_file (Image image, FileData file_data, SaveFlags flags, Cancellable cancellable) throws Error {
		var stream = yield file_data.file.replace_async (file_data.get_etag (),
			false,
			FileCreateFlags.NONE,
			Priority.DEFAULT,
			cancellable);
		yield save_to_stream (image, stream, file_data, flags, cancellable);
		stream.close ();
		// TODO: remove image etag
		image.set_attribute ("ETag", stream.get_etag ());
		file_data.set_etag (stream.get_etag ());
	}

	public async void create_file (Image image, FileData file_data, SaveFlags flags, Cancellable cancellable) throws Error {
		var stream = yield file_data.file.create_async (FileCreateFlags.NONE,
			Priority.DEFAULT,
			cancellable);
		yield save_to_stream (image, stream, file_data, flags, cancellable);
		stream.close ();
		// TODO: remove image etag
		image.set_attribute ("ETag", stream.get_etag ());
		file_data.set_etag (stream.get_etag ());
	}

	async void save_to_stream (Image image, OutputStream stream, FileData file_data, SaveFlags flags, Cancellable cancellable) throws Error {
		var job = new Job ();
		job.callback = save_to_stream.callback;
		job.stream = stream;
		job.file_data = file_data;
		job.image = image;
		job.flags = flags;
		job.cancellable = cancellable;
		factory.add_job (job);
		yield;
		if (job.error != null) {
			throw job.error;
		}
	}

	class Job : Work.Job {
		public OutputStream stream;
		public FileData file_data;
		public Image image;
		public SaveFlags flags;
		public Cancellable cancellable;

		public Job () {
			image = null;
		}

		public override void run (uint8[] tmp_buffer) throws Error {
			var save_func = app.get_save_func (file_data.get_content_type ());
			if (save_func == null) {
				throw new IOError.NOT_SUPPORTED (_("Cannot save this kind of files"));
			}

			if (image.has_icc_profile ()) {
				image = image.dup ();

				// Transform to sRGB
				var srgb = new Gth.IccProfile.sRGB ();
				image.apply_icc_profile (app.color_manager, srgb, cancellable);

				file_data.info.set_attribute_boolean ("Loaded::Image::WasModified", true);

				if (Exiv2.can_write_metadata (file_data.get_content_type ())) {
					// Overwrite Exif.Photo.ColorSpace
					var srgb_tag = new Gth.Metadata.sRGBColorSpace ();
					file_data.info.set_attribute_object ("Exif::Photo::ColorSpace", srgb_tag);
				}
			}
			else if (Exiv2.can_write_metadata (file_data.get_content_type ())) {
				// Remove the Exif.Photo.ColorSpace tag
				file_data.info.remove_attribute ("Exif::Photo::ColorSpace");

				// TODO: allow the user to choose what to do.

				// Assign the sRGB colorspace
				/*var srgb_tag = new Gth.Metadata.sRGBColorSpace ();
				file_data.set_attribute_object ("Exif::Photo::ColorSpace", srgb_tag);*/

				file_data.info.set_attribute_boolean ("Loaded::Image::WasModified", true);
			}

			Gth.Option[] options = null;
			var preferences = app.get_saver_preferences (file_data.get_content_type ());
			if (preferences != null) {
				options = preferences.get_options ();
			}

			var bytes = save_func (image, options, cancellable);
			if (bytes == null) {
				throw new IOError.FAILED ("Save failed");
			}

			if (!(SaveFlags.NO_METADATA in flags)) {
				// Save the metadata from file_data.info
				if (Exiv2.can_write_metadata (file_data.get_content_type ())) {
					var new_bytes = Exiv2.write_metadata_to_buffer (bytes, file_data.info, image);
					bytes = new_bytes;
				}
			}

			// Save to stream
			stream.write_all (bytes.get_data (), null, cancellable);
		}
	}
}

[CCode (has_target = false)]
public delegate Bytes? Gth.SaveFunc (
	Gth.Image image,
	[CCode (array_length = false, array_null_terminated = true)] Gth.Option[]? options,
	Cancellable cancellable) throws Error;

[Flags]
public enum Gth.SaveFlags {
	DEFAULT = 0,
	NO_METADATA,
}
