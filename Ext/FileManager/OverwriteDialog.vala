public class Gth.OverwriteDialog : Object {
	public OverwriteResponse default_response;
	public bool single_file;
	public string new_name;
	public string content_type;
	public bool check_extension;

	public OverwriteDialog (Gth.Window _parent) {
		parent = _parent;
		default_response = OverwriteResponse.OVERWRITE;
		single_file = true;
		new_name = null;
		content_type = null;
		check_extension = false;
		source = null;
		thumbnailer = null;
	}

	const string REQUIRED_ATTRIBUTES = FileAttribute.STANDARD_NAME + "," +
		FileAttribute.STANDARD_DISPLAY_NAME + "," +
		FileAttribute.STANDARD_EDIT_NAME + "," +
		FileAttribute.TIME_MODIFIED + "," +
		FileAttribute.TIME_MODIFIED_USEC;

	public async OverwriteResponse ask_image (Gth.Image image, File file, OverwriteRequest _request, Cancellable cancellable) {
		// TODO: load the destination thumbnail, make the image thumbnail
		request = _request;
		try {
			var info = yield Files.query_info (file, REQUIRED_ATTRIBUTES, cancellable);
			destination = new FileData (file, info);
		}
		catch (Error error) {
			return OverwriteResponse.CANCEL;
		}
		var dialog = new_dialog ();
		return yield ask (dialog, cancellable);
	}

	public async OverwriteResponse ask_file (File source_file, File destination_file, OverwriteRequest _request, Cancellable cancellable) {
		request = _request;
		try {
			var info = yield Files.query_info (destination_file, REQUIRED_ATTRIBUTES, cancellable);
			destination = new FileData (destination_file, info);
			if (destination_file.equal (source_file)) {
				request = OverwriteRequest.SAME_FILE;
			}

			info = yield Files.query_info (source_file, REQUIRED_ATTRIBUTES, cancellable);
			source = new FileData (source_file, info);
		}
		catch (Error error) {
			return OverwriteResponse.CANCEL;
		}
		var dialog = new_dialog ();
		return yield ask (dialog, cancellable);
	}

	async OverwriteResponse ask (Adw.AlertDialog dialog, Cancellable cancellable) {
		load_thumbnails ();
		var response = OverwriteResponse.CANCEL;
		while (true) {
			var response_id = yield dialog.choose (parent, cancellable);
			if (response_id == "overwrite") {
				response = OverwriteResponse.OVERWRITE;
				break;
			}
			if (response_id == "overwrite-all") {
				response = OverwriteResponse.OVERWRITE_ALL;
				break;
			}
			if (response_id == "skip") {
				response = OverwriteResponse.SKIP;
				break;
			}
			if (response_id == "skip-all") {
				response = OverwriteResponse.SKIP_ALL;
				break;
			}
			if (response_id != "rename") {
				response = OverwriteResponse.CANCEL;
				break;
			}

			// Rename
			var rename = new ReadFilename (_("Rename File"), _("_Rename"));
			rename.default_value = destination.info.get_edit_name ();
			rename.check_extension = check_extension;
			rename.check_exists = true;
			rename.folder = destination.file.get_parent ();
			new_name = yield rename.read_value (parent, cancellable);
			if (new_name != null) {
				response = OverwriteResponse.RENAME;
				break;
			}
		}
		thumbnailer.cancel ();
		if (source_thumbnail != null) {
			source_thumbnail.unbind ();
		}
		destination_thumbnail.unbind ();
		return response;
	}

	Adw.AlertDialog new_dialog () {
		var dialog = new Adw.AlertDialog (null, null);
		switch (request) {
		case OverwriteRequest.FILE_EXISTS:
			dialog.heading = _("Replace File?");
			// Translators: %s will be replaced with a filename, do not change it.
			dialog.body = _("A file named “%s” already exists. Do you want to replace it?").printf (destination.info.get_display_name ());
			if (single_file) {
				dialog.add_responses (
					"cancel",  _("_Cancel"),
					"overwrite", _("_Replace"),
					"rename", _("_Rename")
				);
			}
			else {
				dialog.add_responses (
					"cancel",  _("_Cancel"),
					"overwrite-all", _("_Always Replace"),
					"skip-all", _("_Never Replace"),
					"overwrite", _("_Replace"),
					"skip", _("_Skip"),
					"rename", _("_Rename")
				);
				dialog.set_response_appearance ("overwrite-all", Adw.ResponseAppearance.DESTRUCTIVE);
				dialog.set_response_appearance ("skip-all", Adw.ResponseAppearance.SUGGESTED);
			}
			dialog.set_response_appearance ("overwrite", Adw.ResponseAppearance.DESTRUCTIVE);
			break;

		case OverwriteRequest.WRONG_ETAG:
			dialog.heading = _("Replace Content?");
			// Translators: %s will be replaced with a filename, do not change it.
			dialog.body = _("The file “%s” was modified by another application. Do you want to replace it?").printf (destination.info.get_display_name ());
			dialog.add_responses (
				"cancel",  _("_Cancel"),
				"overwrite", _("_Replace"),
				"rename", _("_Rename")
			);
			dialog.set_response_appearance ("overwrite", Adw.ResponseAppearance.DESTRUCTIVE);
			break;

		case OverwriteRequest.SAME_FILE:
			dialog.heading = _("Rename File?");
			// Translators: %s will be replaced with a filename, do not change it.
			dialog.body = _("Source and destination are the same. Do you want to rename “%s”?").printf (destination.info.get_display_name ());
			if (single_file) {
				dialog.add_responses (
					"cancel",  _("_Cancel"),
					"rename", _("_Rename")
				);
			}
			else {
				dialog.add_responses (
					"cancel",  _("_Cancel"),
					"skip-all", _("_Always Skip"),
					"skip", _("_Skip"),
					"rename", _("_Rename")
				);
			}
			break;

		case OverwriteRequest.NONE:
			break;
		}

		dialog.set_response_appearance ("rename", Adw.ResponseAppearance.SUGGESTED);
		dialog.default_response = "rename";
		dialog.close_response = "cancel";
		dialog.extra_child = new_extra_child ();
		return dialog;
	}

	Gtk.Widget new_extra_child () {
		// Main container
		var box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 24);
		box.margin_top = 12;
		box.halign = Gtk.Align.CENTER;

		// Destination
		destination_thumbnail = new Gth.Thumbnail ();
		destination_thumbnail.add_css_class ("frame");
		destination_thumbnail.bind (destination);

		var destination_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 6);
		if (request != OverwriteRequest.SAME_FILE) {
			var label = new Gtk.Label ("<small>%s</small>".printf (_("Content to Replace")));
			label.use_markup = true;
			label.add_css_class ("error");
			label.wrap = true;
			destination_box.append (label);
		}
		destination_box.append (destination_thumbnail);
		box.append (destination_box);

		if (request != OverwriteRequest.SAME_FILE) {
			// Source
			source_thumbnail = new Gth.Thumbnail ();
			source_thumbnail.add_css_class ("frame");
			if (source != null) {
				source_thumbnail.bind (source);
			}

			var label = new Gtk.Label ("<small>%s</small>".printf (_("New Content")));
			label.use_markup = true;
			label.wrap = true;

			var source_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 6);
			source_box.append (label);
			source_box.append (source_thumbnail);
			box.append (source_box);
		}

		return box;
	}

	void load_thumbnails () {
		thumbnailer = new Thumbnailer (parent);
		thumbnailer.requested_size = parent.browser.thumbnail_size;
		destination_thumbnail.size = thumbnailer.requested_size;
		thumbnailer.add (destination);
		if ((source != null) && (source_thumbnail != null)) {
			source_thumbnail.size = thumbnailer.requested_size;
			thumbnailer.add (source);
		}
	}

	Gth.Window parent;
	FileData destination;
	Gth.Thumbnail destination_thumbnail;
	FileData source;
	Gth.Thumbnail source_thumbnail;
	Thumbnailer thumbnailer;
	OverwriteRequest request;
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
