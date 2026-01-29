public class Gth.UpdateThumbnail : Gth.FileOperation {
	public override async void execute (Gth.Window window, File file, Gth.Job job) throws Error {
		try {
			var thumbnail_file = Thumbnailer.get_failed_thumbnail_file (file, FileIntent.READ);
			yield thumbnail_file.delete_async (Priority.DEFAULT, job.cancellable);
		}
		catch (Error error) {
			if (error is IOError.CANCELLED) {
				throw error;
			}
		}
		try {
			var thumbnail_file = Thumbnailer.get_thumbnail_file (file, window.browser.thumbnailer.cache_size, FileIntent.READ, job.cancellable);
			yield thumbnail_file.delete_async (Priority.DEFAULT, job.cancellable);
		}
		catch (Error error) {
			if (error is IOError.CANCELLED) {
				throw error;
			}
		}
		window.browser.update_thumbnail_for_file (file);
	}
}
