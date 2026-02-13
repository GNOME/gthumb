public class Gth.ImageFileOperation : Gth.FileOperation {
	public string content_type;
	public File folder;

	public ImageFileOperation (ImageOperation? _operation) {
		operation = _operation;
		content_type = null;
		folder = null;
	}

	public override async void execute (Gth.Window window, File source, Job job) throws Error {
		// Load the image
		var image = yield app.image_loader.load_file (source, LoadFlags.DEFAULT, job.cancellable);

		// Modify the image
		Image new_image = null;
		if (operation != null) {
			new_image = yield app.image_editor.exec_operation (image, operation, job.cancellable);
			if (new_image == null) {
				throw new IOError.CANCELLED ("Cancelled");
			}
		}
		else {
			new_image = image;
		}

		// Save the image
		var destination = source;
		if (folder != null) {
			var basename = source.get_basename ();
			if ((content_type != null)
				&& (image.info.get_attribute_string (FileAttribute.STANDARD_CONTENT_TYPE) != content_type))
			{
				var saver = app.get_saver_preferences (content_type);
				if (saver != null) {
					var ext = saver.get_default_extension ();
					basename = Util.remove_extension (basename) + "." + ext;
				}
			}
			destination = folder.get_child (basename);
		}
		var file_data = new FileData (destination, image.info);
		file_data.info.set_attribute_boolean (PrivateAttribute.LOADED_IMAGE_WAS_MODIFIED, true);
		if (content_type != null) {
			file_data.info.set_attribute_string (FileAttribute.STANDARD_CONTENT_TYPE, content_type);
		}
		yield create_file (window, new_image, file_data, job);
	}

	async void create_file (Gth.Window window, Image image, FileData file_data, Job job) throws Error {
		try {
			yield app.image_saver.create_file (window, image, file_data, SaveFlags.DEFAULT, job.cancellable);
		}
		catch (Error error) {
			if (error is IOError.EXISTS) {
				yield ask_to_overwrite (window, image, file_data, job);
			}
			else {
				throw error;
			}
		}
	}

	async void ask_to_overwrite (Gth.Window window, Image image, FileData file_data, Job job) throws Error {
		if (overwrite_response == OverwriteResponse.SKIP_ALL) {
			return;
		}
		var overwrite = new OverwriteDialog (window);
		overwrite.check_extension = true;
		overwrite.single_file = single_file;
		if (overwrite_response != OverwriteResponse.OVERWRITE_ALL) {
			overwrite_response = yield overwrite.ask_image (image, file_data.file, OverwriteRequest.FILE_EXISTS, job);
		}
		switch (overwrite_response) {
		case OverwriteResponse.CANCEL:
			throw new IOError.CANCELLED ("Cancelled");
			break;

		case OverwriteResponse.OVERWRITE, OverwriteResponse.OVERWRITE_ALL:
			file_data.set_etag (null);
			yield app.image_saver.replace_file (window, image, file_data, SaveFlags.DEFAULT, job.cancellable);
			break;

		case OverwriteResponse.RENAME:
			file_data.rename_from_display_name (overwrite.new_name);
			yield create_file (window, image, file_data, job);
			break;

		default:
			break;
		}
	}

	ImageOperation operation;
}
