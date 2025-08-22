public class Gth.FileManager {
	public FileManager (Window _window) {
		window = _window;
	}

	public async void delete_files (GenericList<FileData> files, Job job) throws Error {
		// Ask confirmation
		var dialog = new Adw.AlertDialog (_("Delete Permanently?"), null);
		dialog.body_use_markup = true;
		if (files.model.n_items == 1) {
			var file = files.first ();
			dialog.body = _("The file <i>%s</i> will be permanently lost. Do you want to delete it?").printf (file.info.get_display_name ());
		}
		else {
			dialog.body = GLib.ngettext (
				"The %'d selected file will be permanently lost. Do you want to delete it?",
				"The %'d selected files will be permanently lost. Do you want to delete them?",
				files.model.n_items
			).printf (files.model.n_items);
		}
		dialog.add_responses (
			"cancel",  _("_Cancel"),
			"delete", _("_Delete")
		);
		dialog.set_response_appearance ("delete", Adw.ResponseAppearance.DESTRUCTIVE);
		dialog.default_response = "cancel";
		dialog.close_response = "cancel";
		job.opens_dialog ();
		var response = yield dialog.choose (window, job.cancellable);
		job.dialog_closed ();
		if (response == "cancel") {
			throw new IOError.CANCELLED ("Cancelled");
		}

		// Delete
		var operation = new DeleteOperation (window);
		yield operation.delete_files (files, job);
	}

	public async void trash_files (GenericList<File> files, Job job) throws Error {
		var deleted_files = new GenericList<File>();
		try {
			var iter = files.iterator ();
			while (iter.next ()) {
				var file = iter.get ();
				yield file.trash_async (Priority.DEFAULT, job.cancellable);
				job.progress = (float) iter.index () / files.model.n_items;
				deleted_files.model.append (file);
			}
		}
		finally {
			app.monitor.files_deleted (deleted_files);
		}
	}

	public async void copy_files_ask_destination (GenericList<File> files, Job job) throws Error {
		var dialog = new Gtk.FileDialog ();
		dialog.modal = true;
		dialog.title = _("Copy");
		dialog.initial_folder = window.get_current_vfs_folder ();
		var destination = yield dialog.select_folder (window, job.cancellable);
		yield copy_files (files, destination, job);
	}

	public async void copy_files (GenericList<File> files, File destination, Job job) throws Error {
		var operation = new CopyOperation (window);
		try {
			yield operation.copy_files (files, destination, job);
		}
		finally {
			app.monitor.files_created (destination, operation.created_files);
		}
	}

	public async void move_files_ask_destination (GenericList<File> files, Job job) throws Error {
		var dialog = new Gtk.FileDialog ();
		dialog.modal = true;
		dialog.title = _("Move");
		dialog.initial_folder = window.get_current_vfs_folder ();
		var destination = yield dialog.select_folder (window, job.cancellable);
		yield move_files (files, destination, job);
	}

	public async void move_files (GenericList<File> files, File destination, Job job) throws Error {
		var operation = new CopyOperation (window);
		try {
			yield operation.move_files (files, destination, job);
		}
		finally {
			app.monitor.files_deleted (operation.deleted_files);
			app.monitor.files_created (destination, operation.created_files);
		}
	}

	public async void duplicate_files (GenericList<File> files, File destination, Job job) throws Error {
		var operation = new CopyOperation (window);
		try {
			yield operation.duplicate_files (files, destination, job);
		}
		finally {
			app.monitor.files_created (destination, operation.created_files);
		}
	}

	weak Window window;
}


class Gth.CopyOperation {
	public GenericList<File> deleted_files;
	public GenericList<File> created_files;

	public CopyOperation (Window _window) {
		window = _window;
		deleted_files = new GenericList<File> ();
		created_files = new GenericList<File> ();
		last_made_destination = null;
		last_overwrite_response = OverwriteResponse.NONE;
	}

	public async void copy_files (GenericList<File> files, File destination_dir, Job job) throws Error {
		total_files = files.model.get_n_items ();
		current_file = 0;
		foreach (var file in files) {
			yield copy_file (file, destination_dir, CopyFlags.DEFAULT, job);
			current_file++;
		}
	}

	public async void move_files (GenericList<File> files, File destination_dir, Job job) throws Error {
		total_files = files.model.get_n_items ();
		current_file = 0;
		foreach (var file in files) {
			yield copy_file (file, destination_dir, CopyFlags.MOVE, job);
			current_file++;
		}
	}

	public async void duplicate_files (GenericList<File> files, File destination_dir, Job job) throws Error {
		total_files = files.model.get_n_items ();
		current_file = 0;
		foreach (var file in files) {
			yield copy_file (file, destination_dir, CopyFlags.DUPLICATE, job);
			current_file++;
		}
	}

	inline File get_default_destination (File source_file, File destination_dir) {
		return destination_dir.get_child (source_file.get_basename ());
	}

	async void copy_file (File source_file, File destination_dir, CopyFlags flags, Job job) throws Error {
		var other_flags = CopyFlags.DEFAULT;
		if (last_overwrite_response == OverwriteResponse.OVERWRITE_ALL) {
			other_flags |= CopyFlags.OVERWRITE;
		}
		var destination_file = yield copy_file_to_destination (source_file,
			get_default_destination (source_file, destination_dir),
			flags | CopyFlags.ALL_METADATA | other_flags,
			job);

		if (destination_file == null) {
			// Skipped
			return;
		}

		created_files.model.append (destination_file);
		if (CopyFlags.MOVE in flags) {
			deleted_files.model.append (source_file);
		}

		// Comment
		yield copy_file_to_destination (Comment.get_comment_file (source_file),
			Comment.get_comment_file (destination_file),
			flags | CopyFlags.MAKE_DESTINATION | CopyFlags.OVERWRITE | CopyFlags.IGNORE_ERRORS,
			job);

		// XMP sidecar
		yield copy_file_to_destination (Util.get_xmp_sidecar (source_file),
			Util.get_xmp_sidecar (destination_file),
			flags | CopyFlags.OVERWRITE | CopyFlags.IGNORE_ERRORS,
			job);
	}

	async File? copy_file_to_destination (File source_file, File destination_file, CopyFlags flags, Job job) throws Error {
		if (CopyFlags.MAKE_DESTINATION in flags) {
			var destination_dir = destination_file.get_parent ();
			if ((last_made_destination == null) || !destination_dir.equal (last_made_destination)) {
				var source_exists = yield Files.query_exists (source_file, job.cancellable);
				if (!source_exists) {
					return null;
				}
				yield Files.make_directory_async (destination_dir, job.cancellable);
				last_made_destination = destination_dir;
			}
		}
		var saved_as = destination_file;
		try {
			var copy_flags = FileCopyFlags.NONE;
			if (CopyFlags.OVERWRITE in flags) {
				copy_flags |= FileCopyFlags.OVERWRITE;
			}
			if (CopyFlags.ALL_METADATA in flags) {
				copy_flags |= FileCopyFlags.ALL_METADATA;
			}
			if (CopyFlags.MOVE in flags) {
				yield source_file.move_async (destination_file,
					copy_flags,
					Priority.DEFAULT,
					job.cancellable,
					null
				);
			}
			else {
				yield source_file.copy_async (destination_file,
					copy_flags,
					Priority.DEFAULT,
					job.cancellable,
					null
				);
			}
		}
		catch (Error error) {
			if ((error is IOError.EXISTS) && !(CopyFlags.OVERWRITE in flags) && (CopyFlags.DUPLICATE in flags)) {
				var new_destination_file = Util.get_duplicated (destination_file);
				saved_as = yield copy_file_to_destination (source_file, new_destination_file, flags, job);
			}
			else if ((error is IOError.EXISTS) && !(CopyFlags.OVERWRITE in flags)) {
				if (last_overwrite_response == OverwriteResponse.SKIP_ALL) {
					return null;
				}

				var overwrite = new OverwriteDialog (window);
				overwrite.single_file = (total_files == 1);
				last_overwrite_response = yield overwrite.ask_file (source_file,
					destination_file,
					OverwriteRequest.FILE_EXISTS,
					job);

				switch (last_overwrite_response) {
				case OverwriteResponse.CANCEL:
					throw new IOError.CANCELLED ("Cancelled");

				case OverwriteResponse.SKIP, OverwriteResponse.SKIP_ALL, OverwriteResponse.NONE:
					saved_as = null;
					break;

				case OverwriteResponse.OVERWRITE, OverwriteResponse.OVERWRITE_ALL:
					saved_as = yield copy_file_to_destination (source_file, destination_file, flags | CopyFlags.OVERWRITE, job);
					break;

				case OverwriteResponse.RENAME:
					var parent = destination_file.get_parent ();
					var new_destination_file = parent.get_child_for_display_name (overwrite.new_name);
					saved_as = yield copy_file_to_destination (source_file, new_destination_file, flags, job);
					break;
				}
			}
			else {
				var ignore_error = false;
				if (CopyFlags.IGNORE_ERRORS in flags) {
					ignore_error = true;
					// Never ignore some errors.
					if ((error is IOError.NO_SPACE) || (error is IOError.CANCELLED)) {
						ignore_error = false;
					}
				}
				if (!ignore_error) {
					throw error;
				}
			}
		}
		return saved_as;
	}

	weak Window window;
	File last_made_destination;
	uint total_files;
	uint current_file;
	OverwriteResponse last_overwrite_response;
}


[Flags]
public enum Gth.CopyFlags {
	DEFAULT,
	MAKE_DESTINATION,
	OVERWRITE,
	IGNORE_ERRORS,
	MOVE,
	DUPLICATE,
	ALL_METADATA,
}


class Gth.DeleteOperation {
	public DeleteOperation (Window _window) {
		window = _window;
	}

	public async void delete_files (GenericList<FileData> files, Job job) throws Error {
		var deleted_files = new GenericList<File>();
		try {
			var iter = files.iterator ();
			while (iter.next ()) {
				var file = iter.get ().file;
				yield delete_file (file, job.cancellable);
				job.progress = (float) iter.index () / files.model.n_items;
				deleted_files.model.append (file);
			}
		}
		finally {
			app.monitor.files_deleted (deleted_files);
		}
	}

	public async void delete_file (File file, Cancellable cancellable) throws Error {
		yield file.delete_async (Priority.DEFAULT, cancellable);

		// Comment
		try {
			var comment_file = Comment.get_comment_file (file);
			yield comment_file.delete_async (Priority.DEFAULT, cancellable);
		}
		catch (Error error) {
		}

		// XMP sidecar
		try {
			var xmp_file = Util.get_xmp_sidecar (file);
			yield xmp_file.delete_async (Priority.DEFAULT, cancellable);
		}
		catch (Error error) {
		}
	}

	Window window;
}
