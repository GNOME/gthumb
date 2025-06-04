public class Gth.Thumbnailer {
	public enum Size {
		NORMAL = 0,
		LARGE,
		XLARGE,
		XXLARGE;

		public uint to_pixels () {
			return PIXELS[this];
		}

		public unowned string get_subdir () {
			return SUBDIR[this];
		}

		const uint[] PIXELS = {
			128,
			256,
			384,
			512
		};

		const string[] SUBDIR = {
			"normal",
			"large",
			"x-large",
			"xx-large",
		};
	}

	public uint requested_size;
	public Size cache_size;
	public bool load_from_cache;
	public bool save_to_cache;

	public Thumbnailer () {
		cache_size = Size.LARGE;
		load_from_cache = true;
		save_to_cache = true;
		file_queue = new Queue<FileData>();
		job_queue = new GenericArray<ThumbnailJob>();
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
		var local_queue = new GenericArray<ThumbnailJob>();
		foreach (unowned var job in job_queue) {
			local_queue.add (job);
		}
		foreach (unowned var thumbnail_job in local_queue) {
			thumbnail_job.job.cancel ();
		}
		file_queue.clear ();
	}

	void load_next_thumbnail () {
		if (job_queue.length >= app.thumb_loader.factory.n_workers) {
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
				//stdout.printf ("> LOAD THUMBNAIL ERROR: %s\n", error.message);
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
			thumbnail = yield load_thumbnail_from_cache (file_data, job.cancellable);
		}
		if (thumbnail == null) {
			var	valid_failed = yield has_valid_failed_thumbnail (file_data, job.cancellable);
			if (!valid_failed) {
				thumbnail = yield generate_thumbnail (file_data, job.cancellable);
				if (save_to_cache) {
					if (thumbnail != null) {
						yield save_thumbnail_to_cache (file_data, thumbnail, job.cancellable);
					}
					else {
						yield save_failed_thumbnail_to_cache (file_data, job.cancellable);
					}
				}
			}
		}
		if ((thumbnail != null) && (Util.content_type_is_video (file_data.get_content_type ()))) {
			if (filmholes == null) {
				var bytes = GLib.resources_lookup_data ("/app/gthumb/gthumb/icons/filmholes.png", ResourceLookupFlags.NONE);
				filmholes = load_png (bytes, 0, job.cancellable);
			}
			if (filmholes != null) {
				thumbnail.fill_vertical (filmholes, Fill.START);
				thumbnail.fill_vertical (filmholes, Fill.END);
			}
		}
		return thumbnail;
	}

	Gth.Image filmholes = null;

	async Gth.Image? load_thumbnail_from_cache (FileData file_data, Cancellable cancellable) throws Error {
		try {
			var thumbnail_file = Thumbnailer.get_thumbnail_file (file_data.file, cache_size, FileIntent.READ, cancellable);
			var thumbnail = yield app.thumb_loader.load_if_valid (thumbnail_file, file_data, cancellable);
			return yield thumbnail.resize_async (requested_size, ResizeFlags.DEFAULT, ScaleFilter.GOOD, cancellable);
		}
		catch (Error error) {
			//stdout.printf ("> load_thumbnail_from_cache: %s\n", error.message);
			if (error is IOError.CANCELLED)
				throw error;
			return null;
		}
	}

	async Gth.Image? generate_thumbnail (FileData file_data, Cancellable cancellable) throws Error {
		try {
			var image = yield app.image_loader.load_file (file_data.file, cancellable, cache_size.to_pixels ());
			var resized = yield image.resize_async (requested_size, ResizeFlags.UPSCALE, ScaleFilter.GOOD, cancellable);
			set_file_attributes_to_image (resized, file_data);
			resized.set_attribute ("Thumb::Image::Width", "%u".printf (image.get_width ()));
			resized.set_attribute ("Thumb::Image::Height", "%u".printf (image.get_height ()));
			return resized;
		}
		catch (Error error) {
			//stdout.printf ("> generate_thumbnail: %s\n", error.message);
			if (error is IOError.CANCELLED)
				throw error;
			return null;
		}
	}

	async void save_thumbnail_to_cache (FileData file_data, Gth.Image thumbnail, Cancellable cancellable) throws Error {
		try {
			var thumbnail_file = Thumbnailer.get_thumbnail_file (file_data.file, cache_size, FileIntent.WRITE, cancellable);
			yield app.image_saver.save_to_file (thumbnail_file, "image/png", thumbnail, cancellable);
		}
		catch (Error error) {
			//stdout.printf ("> save_thumbnail_to_cache: %s\n", error.message);
			if (error is IOError.CANCELLED)
				throw error;
		}
	}

	async void save_failed_thumbnail_to_cache (FileData file_data, Cancellable cancellable) throws Error {
		try {
			var thumbnail_file = Thumbnailer.get_failed_thumbnail_file (file_data.file, FileIntent.WRITE);
			var thumbnail = new Gth.Image (1, 1);
			set_file_attributes_to_image (thumbnail, file_data);
			yield app.image_saver.save_to_file (thumbnail_file, "image/png", thumbnail, cancellable);
		}
		catch (Error error) {
			//stdout.printf ("> save_failed_thumbnail_to_cache: %s\n", error.message);
			if (error is IOError.CANCELLED)
				throw error;
		}
	}

	async bool has_valid_failed_thumbnail (FileData file_data, Cancellable cancellable) throws Error {
		try {
			var thumbnail_file = Thumbnailer.get_failed_thumbnail_file (file_data.file, FileIntent.READ);
			var stream = yield thumbnail_file.read_async (Priority.DEFAULT, cancellable);
			var bytes = yield Files.read_all_async (stream, cancellable);
			return valid_thumbnail_for_file (bytes, file_data, cancellable);
		}
		catch (Error error) {
			//stdout.printf ("> has_valid_failed_thumbnail: %s\n", error.message);
			if (error is IOError.CANCELLED)
				throw error;
			return false;
		}
	}

	public static bool valid_thumbnail_for_file (Bytes bytes, FileData file_data, Cancellable cancellable) {
		try {
			var attributes = load_png_attributes (bytes);
			if (attributes["Thumb::URI"] != file_data.file.get_uri ()) {
				//stdout.printf ("  valid_thumbnail_for_file[1] %s <=> %s\n",
				//	attributes["Thumb::URI"],
				//	file_data.file.get_uri ());
				return false;
			}
			unowned var embedded_mtime_value = attributes["Thumb::MTime"];
			if (embedded_mtime_value == null) {
				//stdout.printf ("  valid_thumbnail_for_file[2]\n");
				return false;
			}
			int64 embedded_mtime;
			if (!int64.try_parse (embedded_mtime_value, out embedded_mtime, null, 10)) {
				//stdout.printf ("  valid_thumbnail_for_file[3]\n");
				return false;
			}
			var datetime = file_data.info.get_modification_date_time ();
			if (datetime == null) {
				//stdout.printf ("  valid_thumbnail_for_file[4]\n");
				return false;
			}
			return embedded_mtime == datetime.to_unix ();
		}
		catch (Error error) {
			//stdout.printf ("LOAD PNG ERROR: %s\n", error.message);
			return false;
		}
	}

	static void set_file_attributes_to_image (Gth.Image image, FileData file_data) {
		image.set_attribute ("Thumb::URI", file_data.file.get_uri ());
		var datetime = file_data.info.get_modification_date_time ();
		if (datetime != null) {
			image.set_attribute ("Thumb::MTime", ("%" + int64.FORMAT).printf (datetime.to_unix ()));
		}
		image.set_attribute ("Software", "app.gthumb.Thumbnails");
	}

	static File get_thumbnail_file (File file, Size size, FileIntent intent, Cancellable cancellable) throws Error {
		var dir = Files.build_directory (intent,
			File.new_for_path (Environment.get_user_cache_dir ()),
			"thumbnails",
			size.get_subdir ());
		return dir.get_child (Thumbnailer.get_thumbnail_basename (file));
	}

	static File get_failed_thumbnail_file (File file, FileIntent intent) {
		var dir = Files.build_directory (intent,
			File.new_for_path (Environment.get_user_cache_dir ()),
			"thumbnails", "fail", "gnome-thumbnail-factory");
		return dir.get_child (Thumbnailer.get_thumbnail_basename (file));
	}

	static string get_thumbnail_basename (File file) {
		var checksum = new Checksum (ChecksumType.MD5);
		var uri = file.get_uri ();
		checksum.update (uri.data, uri.length);
		return checksum.get_string () + ".png";
	}

	class ThumbnailJob {
		public FileData file;
		public Gth.Job job;

		public ThumbnailJob (FileData _file) {
			file = _file;
			job = app.new_job ("Load Thumbnail for %s".printf (file.file.get_uri ()));
		}
	}

	Queue<FileData> file_queue;
	GenericArray<ThumbnailJob> job_queue;
}
