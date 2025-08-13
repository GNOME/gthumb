public class Gth.OverwriteDialog : Object {
	public OverwriteResponse default_response;
	public bool single_file;
	public string new_name;
	public string content_type;

	public OverwriteDialog (Gtk.Window _parent) {
		parent = _parent;
		default_response = OverwriteResponse.OVERWRITE;
		single_file = true;
		new_name = null;
		content_type = null;
	}

	const string REQUIRED_ATTRIBUTES = FileAttribute.STANDARD_NAME + "," +
		FileAttribute.STANDARD_DISPLAY_NAME + "," +
		FileAttribute.STANDARD_EDIT_NAME;

	public async OverwriteResponse ask_image (Gth.Image image, File destination, OverwriteRequest request, Cancellable cancellable) {
		// TODO: load the destination thumbnail, make the image thumbnail
		try {
			destination_info = yield Files.query_info (destination, REQUIRED_ATTRIBUTES, cancellable);
		}
		catch (Error error) {
			return OverwriteResponse.CANCEL;
		}
		var dialog = new_dialog (request, destination_info);
		return yield ask (dialog, request, cancellable);
	}

	public async OverwriteResponse ask_file (File source, File destination, OverwriteRequest request, Cancellable cancellable) {
		// TODO: load the source and destination thumbnails
		try {
			destination_info = yield Files.query_info (destination, REQUIRED_ATTRIBUTES, cancellable);
			if (destination.equal (source)) {
				request = OverwriteRequest.SAME_FILE;
			}
		}
		catch (Error error) {
			return OverwriteResponse.CANCEL;
		}
		var dialog = new_dialog (request, destination_info);
		return yield ask (dialog, request, cancellable);
	}

	async OverwriteResponse ask (Adw.AlertDialog dialog, OverwriteRequest request, Cancellable cancellable) {
		while (true) {
			var response = yield dialog.choose (parent, cancellable);
			if (response == "overwrite") {
				return OverwriteResponse.OVERWRITE;
			}
			if (response != "rename") {
				return OverwriteResponse.CANCEL;
			}

			// Rename
			var rename = new ReadFilename (_("Rename File"), _("_Rename"));
			rename.default_value = destination_info.get_edit_name ();
			new_name = yield rename.read_value (parent, cancellable);
			if (new_name != null) {
				return OverwriteResponse.RENAME;
			}
		}
	}

	Adw.AlertDialog new_dialog (OverwriteRequest request, FileInfo destination_info) {
		var dialog = new Adw.AlertDialog (null, null);
		switch (request) {
		case OverwriteRequest.FILE_EXISTS:
			dialog.heading = _("File Exists");
			// Translators: %s will be replaced with a filename, do not change it.
			dialog.body = _("A file named “%s” already exists. Do you want to replace it?").printf (destination_info.get_display_name ());
			dialog.add_responses (
				"cancel",  _("_Cancel"),
				"overwrite", _("_Replace"),
				"rename", _("_Rename")
			);
			dialog.set_response_appearance ("overwrite", Adw.ResponseAppearance.DESTRUCTIVE);
			break;

		case OverwriteRequest.WRONG_ETAG:
			dialog.heading = _("File Modified");
			// Translators: %s will be replaced with a filename, do not change it.
			dialog.body = _("The file “%s” was modified by another application. Do you want to replace it?").printf (destination_info.get_display_name ());
			dialog.add_responses (
				"cancel",  _("_Cancel"),
				"overwrite", _("_Replace"),
				"rename", _("_Rename")
			);
			dialog.set_response_appearance ("overwrite", Adw.ResponseAppearance.DESTRUCTIVE);
			break;

		case OverwriteRequest.SAME_FILE:
			dialog.heading = _("Same File");
			// Translators: %s will be replaced with a filename, do not change it.
			dialog.body = _("Source and destination are the same. Do you want to rename “%s”?").printf (destination_info.get_display_name ());
			dialog.add_responses (
				"cancel",  _("_Cancel"),
				"rename", _("_Rename")
			);
			break;

		case OverwriteRequest.NONE:
			break;
		}

		dialog.set_response_appearance ("rename", Adw.ResponseAppearance.SUGGESTED);
		dialog.default_response = "rename";
		dialog.close_response = "cancel";
		return dialog;
	}

	Gtk.Window parent;
	FileInfo destination_info;
}

public enum Gth.OverwriteResponse {
	NONE,
	CANCEL,
	OVERWRITE,
	OVERWRITE_ALL,
	SKIP,
	SKIP_ALL,
	RENAME,
}

public enum Gth.OverwriteRequest {
	NONE,
	FILE_EXISTS,
	WRONG_ETAG,
	SAME_FILE,
}
