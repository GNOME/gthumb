public class Gth.ImageLoader {
	public uint requested_size = 0;

	public async Image? load_from_file (File file, Cancellable cancellable) throws Error {
		var stream = yield file.read_async (Priority.DEFAULT, cancellable);
		return yield load_from_stream (stream, file, cancellable);
	}

	public async Image? load_from_stream (InputStream stream, File? file, Cancellable cancellable) throws Error {
		SourceFunc callback = load_from_stream.callback;
		ThreadFunc<bool> thread_func = () => {
			run (stream, file, cancellable);
			Idle.add ((owned) callback);
			return true;
		};
		new Thread<bool> ("Image.load_from_stream", thread_func);
		yield; // Wait for the thread to finish.
		if (error != null) {
			throw error;
		}
		return image;
	}

	void run (InputStream stream, File? file, Cancellable cancellable) {
		try {
			var content_type = Util.guess_content_type_from_stream (stream, file, cancellable);
			if (content_type == null) {
				throw new IOError.FAILED (_("Unknown file type"));
			}
			var loader_func = app.get_image_loader (content_type);
			if (loader_func == null) {
				throw new IOError.NOT_SUPPORTED (_("No suitable loader available for this file type"));
			}
			var bytes = Files.read_all (stream, cancellable);
			image = loader_func (bytes, requested_size, cancellable);
			// TODO
			//unowned var icc_profile = image.get_icc_profile ();
			//if (icc_profile != null) {
			//	yield image.apply_icc_profile_async (app.get_color_manager (), icc_profile, cancellable);
			//}
		}
		catch (Error local_error) {
			error = local_error;
		}
	}

	Image image = null;
	Error error = null;
}

[CCode (has_target = false)]
public delegate Gth.Image Gth.ImageLoaderFunc (Bytes bytes, uint requested_size, Cancellable cancellable) throws Error;
