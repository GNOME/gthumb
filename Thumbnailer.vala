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

	public uint requested_size;
	public Size cache_size;
	public bool load_from_cache;
	public bool save_to_cache;

	public Thumbnailer (ImageLoader _loader) {
		cache_size = Size.LARGE;
		load_from_cache = true;
		save_to_cache = true;
		file_queue = new Queue<FileData>();
		job_queue = new GenericArray<ThumbnailJob>();
		loader = _loader;
	}

	public void add (FileData file) {
		if (file.thumbnail_state == ThumbnailState.LOADED) {
			return;
		}
		file_queue.push_tail (file);
		load_next_thumbnail ();
	}

	public void remove (FileData file) {
		foreach (unowned var thumbnail_job in job_queue) {
			if (file == thumbnail_job.file) {
				thumbnail_job.job.cancel ();
			}
		}
		file_queue.remove (file);
	}

	public void cancel () {
		foreach (unowned var thumbnail_job in job_queue) {
			thumbnail_job.job.cancel ();
		}
		file_queue.clear ();
	}

	void load_next_thumbnail () {
		if (job_queue.length >= loader.n_workers) {
			return;
		}
		var file = file_queue.pop_head ();
		if (file == null) {
			return;
		}
		var thumbnail_job = new ThumbnailJob (file);
		job_queue.add (thumbnail_job);
		load_thumbnail.begin (thumbnail_job.file, thumbnail_job.job, (_obj, res) => {
			try {
				var thumbnail = load_thumbnail.end (res);
				thumbnail_job.file.set_thumbnail (thumbnail);
			}
			catch (Error error) {
				stdout.printf ("  ERROR: %s\n", error.message);
			}
			thumbnail_job.job.done ();
			job_queue.remove (thumbnail_job);
			load_next_thumbnail ();
		});
		load_next_thumbnail ();
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
		unowned var path = file_data.info.get_attribute_byte_string (cache_size.to_file_attribute ());
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
		var image = yield loader.load_from_file (File.new_for_path (path), cancellable);
		var resized = yield image.resize_if_larger_async (requested_size, ScaleFilter.GOOD, cancellable);
		return resized;
	}

	async Gth.Image? generate_thumbnail (FileData file_data, Cancellable cancellable) throws Error {
		var image = yield loader.load_from_file (file_data.file, cancellable, cache_size.to_pixels ());
		return yield image.resize_if_larger_async (requested_size, ScaleFilter.GOOD, cancellable);
	}

	async void save_thumbnail_to_cache (FileData file_data, Gth.Image image, Cancellable cancellable) throws Error {
		// TODO
	}

	async void save_failed_thumbnail_to_cache (FileData file_data, Cancellable cancellable) throws Error {
		// TODO
	}

	class ThumbnailJob {
		public FileData file;
		public Gth.Job job;

		public ThumbnailJob (FileData _file) {
			file = _file;
			job = app.new_job ("Load thumbnail for %s".printf (file.file.get_uri ()));
		}
	}

	Queue<FileData> file_queue;
	GenericArray<ThumbnailJob> job_queue;
	ImageLoader loader;
}
