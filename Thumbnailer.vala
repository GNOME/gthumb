public class Gth.Thumbnailer {
	public enum Size {
		NORMAL = 0,
		LARGE,
		XLARGE,
		XXLARGE;

		public unowned string to_file_attribute () {
			return FILE_ATTRIBUTE[this];
		}

		public uint to_pixels () {
			return PIXELS[this];
		}

		const string[] FILE_ATTRIBUTE = {
			FileAttribute.THUMBNAIL_PATH_NORMAL,
			FileAttribute.THUMBNAIL_PATH_LARGE,
			FileAttribute.THUMBNAIL_PATH_XLARGE,
			FileAttribute.THUMBNAIL_PATH_XXLARGE,
		};

		const uint[] PIXELS = {
			128,
			256,
			384,
			512
		};
	}

	public Size size;
	public bool load_from_cache;
	public bool save_to_cache;

	public Thumbnailer (ImageLoader _loader) {
		size = Size.LARGE;
		load_from_cache = true;
		save_to_cache = true;
		queue = new Queue<FileData>();
		thumbnail_file = null;
		thumbnail_job = null;
		loader = _loader;
	}

	public void add (FileData file) {
		queue.push_tail (file);
		load_next_thumbnail ();
	}

	public void remove (FileData file) {
		if (file == thumbnail_file) {
			if (thumbnail_job != null) {
				thumbnail_job.cancel ();
			}
		}
		queue.remove (file);
	}

	public void cancel () {
		if (thumbnail_job != null) {
			thumbnail_job.cancel ();
		}
		queue.clear ();
	}

	void load_next_thumbnail () {
		if (thumbnail_job != null) {
			return;
		}
		thumbnail_file = queue.pop_head ();
		if (thumbnail_file == null) {
			return;
		}
		var local_job = app.new_job ("Load thumbnail for %s".printf (thumbnail_file.file.get_uri ()));
		thumbnail_job = local_job;
		//thumbnail_file.set_thumbnail_loading ();
		load_thumbnail.begin (thumbnail_file, local_job, (_obj, res) => {
			try {
				var thumbnail = load_thumbnail.end (res);
				thumbnail_file.set_thumbnail (thumbnail);
			}
			catch (Error error) {
				if (!local_job.is_cancelled ()) {
					//thumbnail_file.set_broken_thumbnail ();
				}
				stdout.printf ("  ERROR: %s\n", error.message);
			}
			local_job.done ();
			if (thumbnail_job == local_job) {
				thumbnail_job = null;
				thumbnail_file = null;
				load_next_thumbnail ();
			}
		});
	}

	async Gth.Image? load_thumbnail (FileData file_data, Job job) throws Error {
		Gth.Image thumbnail = null;
		if (load_from_cache) {
			try {
				thumbnail = yield load_thumbnail_from_cache (file_data, job.cancellable);
			}
			catch (Error error) {
				// Ignore errors.
			}
			if (job.is_cancelled ()) {
				throw new IOError.CANCELLED ("Cancelled");
			}
		}
		if (thumbnail == null) {
			try {
				thumbnail = yield generate_thumbnail (file_data, job.cancellable);
				if (save_to_cache) {
					try {
						yield save_thumbnail_to_cache (file_data, thumbnail, job.cancellable);
					}
					catch (Error error) {
						// Ignore errors.
					}
				}
			}
			catch (Error error) {
				if (save_to_cache) {
					try {
						yield save_failed_thumbnail_to_cache (file_data, job.cancellable);
					}
					catch (Error error) {
						// Ignore errors.
					}
				}
			}
		}
		return thumbnail;
	}

	unowned string get_thumbnail_path (FileData file_data) {
		unowned var path = file_data.info.get_attribute_byte_string (size.to_file_attribute ());
		if (path == null) {
			path = file_data.info.get_attribute_byte_string (FileAttribute.THUMBNAIL_PATH);
		}
		return path;
	}

	async Gth.Image? load_thumbnail_from_cache (FileData file_data, Cancellable cancellable) throws Error {
		unowned var path = get_thumbnail_path (file_data);
		if (path == null) {
			throw new IOError.FAILED ("Thumbnail path not available");
		}
		return yield loader.load_from_file (File.new_for_path (path), cancellable, size.to_pixels ());
	}

	async Gth.Image? generate_thumbnail (FileData file_data, Cancellable cancellable) throws Error {
		var image = yield loader.load_from_file (file_data.file, cancellable, size.to_pixels ());
		//return yield image.resize_if_larger (size.to_pixels (), cancellable);
		return image;
	}

	async void save_thumbnail_to_cache (FileData file_data, Gth.Image image, Cancellable cancellable) throws Error {
		// TODO
	}

	async void save_failed_thumbnail_to_cache (FileData file_data, Cancellable cancellable) throws Error {
		// TODO
	}

	Queue<FileData> queue;
	FileData thumbnail_file;
	Gth.Job thumbnail_job;
	ImageLoader loader;
}
