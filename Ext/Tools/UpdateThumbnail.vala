public class Gth.UpdateThumbnail : Gth.FileOperation {
	public UpdateThumbnail (Gth.Window _window) {
		window = _window;
	}

	public override async void exec (File file, Cancellable cancellable) throws Error {
		try {
			var thumbnail_file = Thumbnailer.get_failed_thumbnail_file (file, FileIntent.READ);
			yield thumbnail_file.delete_async (Priority.DEFAULT, cancellable);
		}
		catch (Error error) {
			if (error is IOError.CANCELLED) {
				throw error;
			}
		}
		try {
			var thumbnail_file = Thumbnailer.get_thumbnail_file (file, window.browser.thumbnailer.cache_size, FileIntent.READ, cancellable);
			yield thumbnail_file.delete_async (Priority.DEFAULT, cancellable);
		}
		catch (Error error) {
			if (error is IOError.CANCELLED) {
				throw error;
			}
		}
		window.browser.update_thumbnail_for_file (file);
	}

	Gth.Window window;
}
