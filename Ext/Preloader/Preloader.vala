public class Gth.Preloader {
	public ImageCache cache;

	public Preloader () {
		cache = new ImageCache ();
		last_job = null;
	}

	public async void load (Gth.MonitorProfile? monitor_profile, Queue<File> queue, Job job,
		uint requested_size = 0) throws Error {
		if (last_job != null) {
			last_job.cancellable.cancel ();
		}
		last_job = job;
		try {
			while (!queue.is_empty ()) {
				var file = queue.pop_head ();
				var image = yield app.image_loader.load_file (monitor_profile, file,
					LoadFlags.NO_BIG_IMAGES, job.cancellable, requested_size);
				cache.add (file, image);
			}
		}
		catch (Error error) {
		}
		if (last_job == job) {
			last_job = null;
		}
	}

	public void cancel () {
		if (last_job != null) {
			last_job.cancellable.cancel ();
		}
	}

	Job last_job;
}
