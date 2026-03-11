public class Gth.UpdateThumbnail : Gth.FileOperation {
	public override async void execute (Gth.MainWindow window, File file, Gth.Job job) throws Error {
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
			var cache_size = window.browser.file_grid.get_thumbnailer_cache_size ();
			var thumbnail_file = Thumbnailer.get_thumbnail_file (file, cache_size, FileIntent.READ, job.cancellable);
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
