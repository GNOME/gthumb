public class Gth.OverwriteDialog : Object {
	public OverwriteResponse default_response;
	public bool single_file;
	public string new_name;
	// public string content_type;
	public bool check_extension;

	public OverwriteDialog (Gth.MainWindow _parent) {
		parent = _parent;
		default_response = OverwriteResponse.OVERWRITE;
		single_file = true;
		new_name = null;
		// content_type = null;
		check_extension = false;
		source = null;
		thumbnailer = null;
	}

	const string REQUIRED_ATTRIBUTES = FileAttribute.STANDARD_NAME + "," +
		FileAttribute.STANDARD_DISPLAY_NAME + "," +
		FileAttribute.STANDARD_EDIT_NAME + "," +
		FileAttribute.TIME_MODIFIED + "," +
		FileAttribute.TIME_MODIFIED_USEC;

	public async OverwriteResponse ask_image (Gth.Image _image, File file, OverwriteRequest _request, Job job) {
		try {
			request = _request;
			var info = yield Files.query_info (file, REQUIRED_ATTRIBUTES, job.cancellable);
			destination = new FileData (file, info);
			source = null;
			image = _image;
			img_data = new FileData (File.new_for_uri ("unused://"));
			img_data.info.set_symbolic_icon (new ThemedIcon ("gth-image-symbolic"));
			var dialog = new_dialog ();
			return yield ask (dialog, job);
		}
		catch (Error error) {
			return OverwriteResponse.CANCEL;
		}
	}

	public async OverwriteResponse ask_file (File source_file, File destination_file, OverwriteRequest _request, Job job) {
		try {
			request = _request;
			var info = yield Files.query_info (destination_file, REQUIRED_ATTRIBUTES, job.cancellable);
			destination = new FileData (destination_file, info);
			if (destination_file.equal (source_file)) {
				request = OverwriteRequest.SAME_FILE;
				source = null;
			}
			else {
				info = yield Files.query_info (source_file, REQUIRED_ATTRIBUTES, job.cancellable);
				source = new FileData (source_file, info);
			}
			var dialog = new_dialog ();
			return yield ask (dialog, job);
		}
		catch (Error error) {
			return OverwriteResponse.CANCEL;
		}
	}

	async OverwriteResponse ask (Adw.AlertDialog dialog, Job job) {
		load_thumbnails ();
		var response = OverwriteResponse.CANCEL;
		while (true) {
			job.opens_dialog ();
			var response_id = yield dialog.choose (parent, job.cancellable);
			job.dialog_closed ();
			var for_all = (for_all_checkbutton != null) && for_all_checkbutton.active;
			if (response_id == "overwrite") {
				response = for_all ? OverwriteResponse.OVERWRITE_ALL : OverwriteResponse.OVERWRITE;
				break;
			}
			if (response_id == "skip") {
				response = for_all ? OverwriteResponse.SKIP_ALL : OverwriteResponse.SKIP;
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
			if (for_all) {
				response = OverwriteResponse.RENAME_ALL;
				break;
			}

			// Rename

			var rename = new ReadFilename (_("Rename File"), _("_Rename"));
			rename.default_value = destination.info.get_edit_name ();
			rename.check_extension = check_extension;
			rename.check_exists = true;
			rename.add_filename_generator ();
			rename.folder = destination.file.get_parent ();
			try {
				new_name = yield rename.read_value (parent, job);
				response = OverwriteResponse.RENAME;
				break;
			}
			catch (Error error) {
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
					"skip", _("_Skip"),
					"overwrite", _("_Replace"),
					"rename", _("_Rename")
				);
			}
			dialog.set_response_appearance ("overwrite", Adw.ResponseAppearance.DESTRUCTIVE);
			dialog.set_response_appearance ("rename", Adw.ResponseAppearance.SUGGESTED);
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
			dialog.set_response_appearance ("rename", Adw.ResponseAppearance.SUGGESTED);
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
					"skip-all", _("Skip _All"),
					"skip", _("_Skip"),
					"rename", _("_Rename")
				);
			}
			dialog.set_response_appearance ("rename", Adw.ResponseAppearance.SUGGESTED);
			break;

		case OverwriteRequest.NONE:
			break;
		}

		dialog.default_response = "rename";
		dialog.close_response = "cancel";
		dialog.extra_child = new_extra_child ();
		return dialog;
	}

	Gtk.Widget new_extra_child () {
		// Main container
		var main_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 24);
		main_box.margin_top = 12;
		//main_box.halign = Gtk.Align.CENTER;

		// Preview container
		var preview_box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 24);
		preview_box.halign = Gtk.Align.CENTER;
		main_box.append (preview_box);

		// Destination
		destination_thumbnail = new Gth.Thumbnail ();
		destination_thumbnail.add_css_class ("image-view");
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
		preview_box.append (destination_box);

		if (request != OverwriteRequest.SAME_FILE) {
			// Source
			source_thumbnail = new Gth.Thumbnail ();
			source_thumbnail.add_css_class ("image-view");
			if (source != null) {
				source_thumbnail.bind (source);
			}
			else if (img_data != null) {
				source_thumbnail.bind (img_data);
			}

			var label = new Gtk.Label ("<small>%s</small>".printf (_("New Content")));
			label.use_markup = true;
			label.wrap = true;

			var source_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 6);
			source_box.append (label);
			source_box.append (source_thumbnail);
			preview_box.append (source_box);
		}

		if (!single_file && (request != OverwriteRequest.SAME_FILE)) {
			for_all_checkbutton = new Gtk.CheckButton.with_label (_("Same answer for all the files"));
			main_box.append (for_all_checkbutton);
		}

		return main_box;
	}

	void load_thumbnails () {
		thumbnailer = new Thumbnailer.for_window (parent);
		thumbnailer.requested_size = parent.browser.file_grid.thumbnail_size;
		thumbnailer.set_active (true);
		destination_thumbnail.size = thumbnailer.requested_size;
		thumbnailer.add (destination);
		if (source_thumbnail != null) {
			source_thumbnail.size = thumbnailer.requested_size;
			if (source != null) {
				thumbnailer.add (source);
			}
			else {
				var resized = image.resize (source_thumbnail.size, ResizeFlags.DEFAULT, ScaleFilter.GOOD);
				img_data.set_thumbnail (resized, source_thumbnail.size);
			}
		}
	}

	Gth.MainWindow parent;
	FileData destination;
	Gth.Thumbnail destination_thumbnail;
	FileData source;
	Gth.Image image;
	FileData img_data;
	Gth.Thumbnail source_thumbnail;
	Thumbnailer thumbnailer;
	OverwriteRequest request;
	Gtk.CheckButton for_all_checkbutton = null;
}

public enum Gth.OverwriteResponse {
	NONE,
	CANCEL,
	OVERWRITE,
	OVERWRITE_ALL,
	SKIP,
	SKIP_ALL,
	RENAME,
	RENAME_ALL,
}

public enum Gth.OverwriteRequest {
	NONE,
	FILE_EXISTS,
	WRONG_ETAG,
	SAME_FILE,
}
