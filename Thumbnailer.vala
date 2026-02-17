public class Gth.Thumbnailer {
	public enum Size {
		NORMAL = 0,
		LARGE,
		XLARGE,
		XXLARGE;

		public uint to_pixels () {
			return PIXELS[this];
		}

		public static Size from_pixel_size (uint pixel_size) {
			for (var i = 0; i < PIXELS.length; i++) {
				if (PIXELS[i] >= pixel_size) {
					return (Size) i;
				}
			}
			return XXLARGE;
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

	public uint requested_size {
		get { return _requested_size; }
		set {
			_requested_size = value;
			cache_size = Size.from_pixel_size (_requested_size);
		}
	}

	public bool load_from_cache;
	public bool save_to_cache;
	public NextFileFunc get_next_file_func;

	public Thumbnailer (Gth.MonitorProfile _monitor_profile, Gth.JobQueue? _app_jobs = null) {
		monitor_profile = _monitor_profile;
		app_jobs = _app_jobs ?? app.jobs;
		requested_size = 256;
		load_from_cache = true;
		save_to_cache = true;
		get_next_file_func = null;
		file_queue = new Queue<FileData>();
		job_queue = new GenericArray<ThumbnailJob>();
		active = true;
	}

	public Thumbnailer.for_window (Gth.Window window) {
		this (window.monitor_profile, window.jobs);
	}

	public void add (FileData file, bool high_priority = false) {
		if (already_added (file)) {
			return;
		}
		if (high_priority) {
			file_queue.push_head (file);
		}
		else {
			file_queue.push_tail (file);
		}
		load_next ();
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
		cancel_load_next ();
		file_queue.clear ();
	}

	public bool already_added (FileData file) {
		return (((file.thumbnail_state == ThumbnailState.LOADED) && (file.thumbnail_size == cache_size.to_pixels ()))
			|| (file.thumbnail_state == ThumbnailState.LOADING)
			|| (file.thumbnail_state == ThumbnailState.BROKEN));
	}

	public void queue_load_next () {
		if (load_event != 0) {
			return;
		}
		load_event = Util.after_next_rearrange (() => {
			load_next ();
			load_event = 0;
		});
	}

	public void set_active (bool _active) {
		active = _active;
	}

	uint load_event = 0;

	void cancel_load_next () {
		if (load_event != 0) {
			Source.remove (load_event);
			load_event = 0;
		}
	}

	void load_next () {
		if (!active) {
			return;
		}
		if (job_queue.length >= app.thumb_loader.factory.n_workers) {
			return;
		}
		FileData file = null;
		if ((file == null) && (get_next_file_func != null)) {
			file = get_next_file_func ();
			// if (file != null) {
			// 	stdout.printf ("> LOAD THUMBNAIL [next]: %s\n", file.file.get_basename ());
			// }
		}
		if (file == null) {
			while (true) {
				file = file_queue.pop_head ();
				if (file == null) {
					break;
				}
				if (file.has_thumbnail ()) {
					continue;
				}
				// stdout.printf ("> LOAD THUMBNAIL [queue]: %s\n", file.file.get_basename ());
				break;
			}
		}
		if (file == null) {
			return;
		}
		var thumbnail_job = new ThumbnailJob (app_jobs, file);
		job_queue.add (thumbnail_job);
		file.thumbnail_state = ThumbnailState.LOADING;
		load_thumbnail.begin (thumbnail_job.file, thumbnail_job.job, (_obj, res) => {
			try {
				var thumbnail = load_thumbnail.end (res);
				thumbnail_job.file.set_thumbnail (thumbnail, cache_size.to_pixels ());
			}
			catch (Error error) {
				//stdout.printf ("> LOAD THUMBNAIL ERROR: %s\n", error.message);
				file.thumbnail_state = (error is IOError.CANCELLED) ? ThumbnailState.ICON : ThumbnailState.BROKEN;
			}
			thumbnail_job.job.done ();
			job_queue.remove (thumbnail_job);
			load_next ();
		});
		load_next ();
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
		if (thumbnail == null) {
			throw new IOError.FAILED ("Could not load or generate a thumbnail for %s".printf (file_data.file.get_uri ()));
		}
		if (Util.content_type_is_video (file_data.get_content_type ())) {
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
			var thumbnail = yield app.thumb_loader.load_if_valid (monitor_profile, thumbnail_file, file_data, cancellable);
			return yield thumbnail.resize_async (_requested_size, ResizeFlags.DEFAULT, ScaleFilter.GOOD, cancellable);
		}
		catch (Error error) {
			//stdout.printf ("> load_thumbnail_from_cache %s: %s\n", file_data.file.get_uri (), error.message);
			if (error is IOError.CANCELLED) {
				throw error;
			}
			return null;
		}
	}

	async Gth.Image? generate_thumbnail (FileData file_data, Cancellable cancellable) throws Error {
		try {
			var image = yield app.image_loader.load_file (monitor_profile, file_data.file, LoadFlags.NO_METADATA, cancellable, cache_size.to_pixels ());
			var resized = yield image.resize_async (cache_size.to_pixels (), ResizeFlags.DEFAULT, ScaleFilter.GOOD, cancellable);
			set_file_attributes_to_image (resized, file_data);
			resized.set_attribute ("Thumb::Image::Width", "%u".printf (image.get_width ()));
			resized.set_attribute ("Thumb::Image::Height", "%u".printf (image.get_height ()));
			return resized;
		}
		catch (Error error) {
			//stdout.printf ("> generate_thumbnail: %s\n", error.message);
			if (error is IOError.CANCELLED) {
				throw error;
			}
			return null;
		}
	}

	async void save_thumbnail_to_cache (FileData original, Gth.Image thumbnail_image, Cancellable cancellable) throws Error {
		try {
			var thumbnail_file = Thumbnailer.get_thumbnail_file (original.file, cache_size, FileIntent.WRITE, cancellable);
			var thumbnail_file_data = new FileData.for_file (thumbnail_file, "image/png");
			yield app.image_saver.replace_file (monitor_profile, thumbnail_image, thumbnail_file_data, SaveFlags.NO_METADATA, cancellable);
		}
		catch (Error error) {
			//stdout.printf ("> save_thumbnail_to_cache %s: %s\n", original.file.get_uri (), error.message);
			if (error is IOError.CANCELLED) {
				throw error;
			}
		}
	}

	async void save_failed_thumbnail_to_cache (FileData original, Cancellable cancellable) throws Error {
		try {
			var thumbnail_image = new Gth.Image (1, 1);
			// TODO: move to thumbnail_file_data?
			set_file_attributes_to_image (thumbnail_image, original);

			var thumbnail_file = Thumbnailer.get_failed_thumbnail_file (original.file, FileIntent.WRITE);
			var thumbnail_file_data = new FileData.for_file (thumbnail_file, "image/png");
			yield app.image_saver.replace_file (monitor_profile, thumbnail_image, thumbnail_file_data, SaveFlags.NO_METADATA, cancellable);
		}
		catch (Error error) {
			//stdout.printf ("> save_failed_thumbnail_to_cache: %s\n", error.message);
			if (error is IOError.CANCELLED) {
				throw error;
			}
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
			if (error is IOError.CANCELLED) {
				throw error;
			}
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
		image.set_attribute ("Software", Config.APP_NAME + " " + Config.APP_VERSION);
	}

	public static File get_thumbnail_file (File file, Size size, FileIntent intent, Cancellable cancellable) throws Error {
		var dir = Files.build_directory (intent,
			File.new_for_path (Environment.get_user_cache_dir ()),
			"thumbnails",
			size.get_subdir ());
		return dir.get_child (Thumbnailer.get_thumbnail_basename (file));
	}

	public static File get_failed_thumbnail_file (File file, FileIntent intent) {
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

		public ThumbnailJob (Gth.JobQueue jobs, FileData _file) {
			file = _file;
			job = jobs.new_job ("Thumbnail for %s".printf (file.get_display_name ()),
				JobFlags.DEFAULT,
				"gth-image-symbolic");
		}
	}

	~Thumbnailer () {
		cancel ();
	}

	uint _requested_size;
	public Size cache_size;
	weak Gth.Browser browser;
	weak Gth.JobQueue app_jobs;
	weak Gth.MonitorProfile monitor_profile;
	Queue<FileData> file_queue;
	GenericArray<ThumbnailJob> job_queue;
	bool active;
}

public delegate Gth.FileData? Gth.NextFileFunc ();
