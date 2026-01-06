public class Gth.FileManager {
	public FileManager (Window _window) {
		window = _window;
	}

	public async void delete_files_from_disk (GenericList<FileData> files, Job job) throws Error {
		// Ask confirmation
		var dialog = new Adw.AlertDialog (_("Delete Permanently?"), null);
		dialog.body_use_markup = true;
		if (files.model.n_items == 1) {
			var file = files.first ();
			dialog.body = _("This action is not reversable. Do you want to delete <i>%s</i>?").printf (
				Markup.escape_text (file.info.get_display_name ()));
		}
		else {
			dialog.body = GLib.ngettext (
				"This action is not reversable. Do you want to delete the selected file?",
				"This action is not reversable. Do you want to delete the %'d selected files?",
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
		var file_list = new GenericList<File>();
		foreach (var file_data in files) {
			file_list.model.append (file_data.file);
		}
		yield operation.delete_files_from_disk (file_list, job);
	}

	public async void trash_files (GenericList<File> files, Job job) throws Error {
		var deleted_files = new GenericList<File>();
		try {
			var iter = files.iterator ();
			while (iter.next ()) {
				var file = iter.get ();
				job.progress = Util.calc_progress (iter.index (), files.model.n_items);
				job.subtitle = file.get_basename ();
				yield file.trash_async (Priority.DEFAULT, job.cancellable);
				deleted_files.model.append (file);
			}
		}
		finally {
			app.monitor.files_deleted_from_disk (deleted_files);
		}
	}

	public async void copy_files_ask_destination (GenericList<File> files, Job job) throws Error {
		var dialog = new Gtk.FileDialog ();
		dialog.modal = true;
		dialog.title = _("Copy");
		dialog.initial_folder = window.get_current_vfs_folder ();
		job.opens_dialog ();
		var destination = yield dialog.select_folder (window, job.cancellable);
		job.dialog_closed ();
		yield copy_files (files, destination, job);
	}

	public async void copy_files (GenericList<File> files, File destination, Job job) throws Error {
		var operation = new CopyOperation (window);
		try {
			yield operation.copy_files (files, destination, job);
		}
		finally {
			app.monitor.files_added_or_changed (destination, operation.created_files);
		}
	}

	public async void move_files_ask_destination (GenericList<File> files, Job job) throws Error {
		var dialog = new Gtk.FileDialog ();
		dialog.modal = true;
		dialog.title = _("Move");
		dialog.initial_folder = window.get_current_vfs_folder ();
		job.opens_dialog ();
		var destination = yield dialog.select_folder (window, job.cancellable);
		job.dialog_closed ();
		yield move_files (files, destination, job);
	}

	public async void move_files (GenericList<File> files, File destination, Job job) throws Error {
		var operation = new CopyOperation (window);
		try {
			yield operation.move_files (files, destination, job);
		}
		finally {
			app.monitor.files_deleted_from_disk (operation.deleted_files);
			app.monitor.files_added_or_changed (destination, operation.created_files);
		}
	}

	public async void duplicate_files (GenericList<File> files, File destination, Job job) throws Error {
		var operation = new CopyOperation (window);
		try {
			yield operation.duplicate_files (files, destination, job);
		}
		finally {
			app.monitor.files_added_or_changed (destination, operation.created_files);
		}
	}

	public async void rename_files (GenericList<RenamedFile> files, Job job) throws Error {
		var operation = new CopyOperation (window);
		try {
			yield operation.rename_files (files, job);
		}
		finally {
			app.monitor.files_renamed (operation.renamed_files);
		}
	}

	public static async GenericList<FileData> query_list_info (GenericList<File> files, string attributes, QueryListFlags flags, Cancellable cancellable) throws Error {
		var file_attributes = Util.extract_file_attributes (attributes);
		var metadata_attributes_v = Util.extract_metadata_attributes (attributes);
		var list = new GenericList<FileData>();
		foreach (var file in files) {
			//stdout.printf ("> query_list_info: %s\n", file.get_uri ());
			var info = yield Files.query_info (file, file_attributes, cancellable);
			switch (info.get_file_type ()) {
			case FileType.REGULAR:
				var file_data = new Gth.FileData (file, info);
				if (metadata_attributes_v.length > 0) {
					yield app.metadata_reader.update (file_data, metadata_attributes_v, cancellable);
				}
				list.model.append (file_data);
				break;

			case FileType.DIRECTORY:
				if (QueryListFlags.NOT_RECURSIVE in flags) {
					list.model.append (new Gth.FileData (file, info));
				}
				else {
					yield FileManager.foreach_child (file, ForEachFlags.RECURSIVE, attributes, cancellable, (child, is_parent) => {
						//stdout.printf ("   append: %s\n", child.file.get_uri ());
						list.model.append (child);
						return ForEachAction.CONTINUE;
					});
				}
				break;

			default:
				break;
			}
		}
		return list;
	}

	const int REMOTE_FILES_PER_REQUEST = 100;
	const int LOCAL_FILES_PER_REQUEST = 1000;

	public static async void foreach_child (File parent, ForEachFlags flags, string attributes, Cancellable cancellable, ForEachChildFunc child_func) throws Error {
		var all_attributes = Util.concat_attributes (REQUIRED_ATTRIBUTES, attributes);
		var metadata_attributes_v = Util.extract_metadata_attributes (all_attributes);
		var has_symbolic_icon = Util.attributes_match_all_patterns (FileAttribute.STANDARD_SYMBOLIC_ICON, all_attributes);
		var read_metadata = ForEachFlags.READ_METADATA in flags;
		var file_attributes = Util.extract_file_attributes (all_attributes);
		var parent_info = yield parent.query_info_async (file_attributes, FileQueryInfoFlags.NONE, Priority.DEFAULT, cancellable);
		if (parent_info.get_file_type () != FileType.DIRECTORY) {
			throw new IOError.FAILED ("Not a directory");
		}

		var queue = new Queue<FileData>();
		queue.push_tail (new Gth.FileData (parent, parent_info));

		var is_local = parent.get_uri_scheme () == "file";
		var files_per_request = is_local ? LOCAL_FILES_PER_REQUEST : REMOTE_FILES_PER_REQUEST;
		Error error = null;
		while (queue.length > 0) {
			var folder_data = queue.pop_head ();
			var action = child_func (folder_data, true);
			if (action == ForEachAction.SKIP) {
				continue;
			}
			if (action == ForEachAction.STOP) {
				break;
			}
			FileEnumerator enumerator = null;
			try {
				var enum_flags = (ForEachFlags.NOFOLLOW_LINKS in flags) ? FileQueryInfoFlags.NOFOLLOW_SYMLINKS : FileQueryInfoFlags.NONE;
				enumerator = yield folder_data.file.enumerate_children_async (file_attributes, enum_flags, Priority.DEFAULT, cancellable);
			}
			catch (Error _error) {
				error = _error;
				action = ForEachAction.STOP;
				break;
			}
			while (action != ForEachAction.STOP) {
				List<FileInfo> info_list = null;
				try {
					info_list = yield enumerator.next_files_async (files_per_request, Priority.DEFAULT, cancellable);
					if (info_list == null) {
						break;
					}
				}
				catch (Error _error) {
					error = _error;
					action = ForEachAction.STOP;
					break;
				}
				foreach (var info in info_list) {
					var child = enumerator.get_child (info);
					var child_data = new Gth.FileData (child, info);

					if (read_metadata
						&& !has_symbolic_icon
						&& (info.get_file_type () == FileType.DIRECTORY))
					{
						// Always set the symbolic icon for directories.
						try {
							var more_info = yield child.query_info_async (
								FileAttribute.STANDARD_SYMBOLIC_ICON,
								FileQueryInfoFlags.NONE,
								Priority.DEFAULT,
								cancellable);
							if (more_info.has_attribute (FileAttribute.STANDARD_SYMBOLIC_ICON)) {
								info.set_symbolic_icon (more_info.get_symbolic_icon ());
							}
						}
						catch (Error _error) {
							error = _error;
							action = ForEachAction.STOP;
							break;
						}
					}

					var child_action = child_func (child_data, false);
					if (child_action == ForEachAction.STOP) {
						action = ForEachAction.STOP;
						break;
					}
					if (child_action == ForEachAction.SKIP) {
						continue;
					}
					if ((info.get_file_type () == FileType.DIRECTORY)
						&& (ForEachFlags.RECURSIVE in flags))
					{
						queue.push_tail (child_data);
					}
					else if (read_metadata
						&& (info.get_file_type () == FileType.REGULAR)
						&& (metadata_attributes_v.length > 0))
					{
						try {
							yield app.metadata_reader.update (child_data, metadata_attributes_v, cancellable);
						}
						catch (Error _error) {
							error = _error;
							action = ForEachAction.STOP;
						}
					}
					if ((error == null) && cancellable.is_cancelled ()) {
						error = new IOError.CANCELLED ("Cancelled");
						action = ForEachAction.STOP;
						break;
					}
				}
			}
			if (action == ForEachAction.STOP) {
				break;
			}
		}
		if (error != null) {
			throw error;
		}
	}

	weak Window window;
}

class Gth.CopyOperation {
	public GenericList<File> deleted_files;
	public GenericList<File> created_files;
	public GenericList<RenamedFile> renamed_files;

	public CopyOperation (Window _window) {
		window = _window;
		deleted_files = new GenericList<File> ();
		created_files = new GenericList<File> ();
		renamed_files = new GenericList<RenamedFile> ();
		copied_sources = new GenericSet<File> (Util.file_hash, Util.file_equal);
		last_made_destination = null;
		last_overwrite_response = OverwriteResponse.NONE;
	}

	public async void copy_files (GenericList<File> files, File destination_dir, Job job) throws Error {
		yield copy_files_generic (files, destination_dir, CopyFlags.DEFAULT, job);
	}

	public async void move_files (GenericList<File> files, File destination_dir, Job job) throws Error {
		yield copy_files_generic (files, destination_dir, CopyFlags.MOVE, job);
	}

	public async void duplicate_files (GenericList<File> files, File destination_dir, Job job) throws Error {
		total_files = files.model.get_n_items ();
		current_file = 0;
		foreach (var file in files) {
			job.subtitle = file.get_basename ();
			job.progress = Util.calc_progress (current_file, total_files);
			yield copy_file (file,
				get_default_destination (file, destination_dir),
				CopyFlags.DUPLICATE,
				job);
			current_file++;
		}
	}

	public async void rename_files (GenericList<RenamedFile> files, Job job) throws Error {
		total_files = files.model.get_n_items ();
		current_file = 0;
		foreach (var renamed_file in files) {
			job.subtitle = renamed_file.old_file.get_basename ();
			job.progress = Util.calc_progress (current_file, total_files);
			yield copy_file (renamed_file.old_file,
				renamed_file.new_file,
				CopyFlags.RENAME,
				job);
			current_file++;
		}
	}

	async void copy_files_generic (GenericList<File> _files, File destination_dir, CopyFlags copy_flags, Job job) throws Error {
		var explicitly_requested = new GenericSet<File> (Util.file_hash, Util.file_equal);
		var files = new GenericList<File> ();
		foreach (var file in _files) {
			var original_dir = file.get_parent ();
			// Ignore if source and destination are the same.
			if ((original_dir == null) || !original_dir.equal (destination_dir)) {
				files.model.append (file);
				explicitly_requested.add (file);
			}
		}
		if (files.length () == 0) {
			return;
		}
		var attributes = REQUIRED_ATTRIBUTES + "," + FileAttribute.STANDARD_SIZE;
		var moving = CopyFlags.MOVE in copy_flags;
		var file_data_list = yield FileManager.query_list_info (files, attributes, QueryListFlags.DEFAULT, job.cancellable);
		total_bytes = 0;
		foreach (var file_data in file_data_list) {
			total_bytes += (uint64) file_data.info.get_size ();
		}
		copied_bytes = 0;
		total_files = file_data_list.model.get_n_items ();
		current_file = 0;
		var moved_directories = new GenericList<File>();
		File source_base_dir = null;
		foreach (var file_data in file_data_list) {
			if (explicitly_requested.contains (file_data.file)) {
				source_base_dir = file_data.file.get_parent ();
			}
			job.progress = Util.calc_progress (copied_bytes, total_bytes);
			job.subtitle = file_data.info.get_display_name ();
			// Ignore already copied files. These are sidecars already copied with copy_file()
			if (!copied_sources.contains (file_data.file)) {
				switch (file_data.info.get_file_type ()) {
				case FileType.DIRECTORY:
					var new_dir = Util.build_destination_dir (file_data.file,
						source_base_dir, destination_dir);
					//stdout.printf ("> mkdir: %s\n", new_dir.get_uri ());
					yield Files.make_directory_async (new_dir, job.cancellable);
					copied_sources.add (file_data.file);
					if (moving) {
						moved_directories.model.append (file_data.file);
					}
					break;

				case FileType.REGULAR:
					try {
						var new_destination_dir = Util.build_destination_dir (file_data.file.get_parent (),
							source_base_dir, destination_dir);
						//stdout.printf ("> copy: %s -> %s\n", file_data.file.get_uri (), new_destination_dir.get_uri ());
						yield copy_file (file_data.file,
							get_default_destination (file_data.file, new_destination_dir),
							copy_flags,
							job);
					}
					catch (Error error) {
						// Ignore NOT_FOUND errors when moving files.
						// These are sidecar files already moved in copy_file()
						if (moving && !(error is IOError.NOT_FOUND)) {
							throw error;
						}
					}
					break;

				default:
					throw new IOError.FAILED ("Cannot copy or move this kind of files: %s (%s)".printf (
						file_data.file.get_uri (),
						file_data.info.get_file_type ().to_string ()));
				}
			}
			copied_bytes += (uint64) file_data.info.get_size ();
			current_file++;
		}

		// Remove the moved directories.
		var iter = moved_directories.iterator ();
		while (iter.previous ()) {
			var dir = iter.get ();
			try {
				dir.delete (job.cancellable);
			}
			catch (Error error) {
				// Ignore this kind of errors, because
				// it is caused by the user choice of
				// not move a file.
				if (!(error is IOError.NOT_EMPTY)) {
					throw error;
				}
			}
		}
	}

	inline File get_default_destination (File source_file, File destination_dir) {
		return destination_dir.get_child (source_file.get_basename ());
	}

	async void copy_file (File source_file, File default_destination_file, CopyFlags flags, Job job) throws Error {
		var other_flags = CopyFlags.DEFAULT;
		if (last_overwrite_response == OverwriteResponse.OVERWRITE_ALL) {
			other_flags |= CopyFlags.OVERWRITE;
		}
		var destination_file = yield copy_file_to_destination (source_file,
			default_destination_file,
			flags | CopyFlags.ALL_METADATA | CopyFlags.UPDATE_PROGRESS | other_flags,
			job);

		if (destination_file == null) {
			// Skipped
			return;
		}

		if (CopyFlags.RENAME in flags) {
			renamed_files.model.append (new RenamedFile (source_file, destination_file));
		}
		else {
			created_files.model.append (destination_file);
			if (CopyFlags.MOVE in flags) {
				deleted_files.model.append (source_file);
			}
		}
		copied_sources.add (source_file);

		// Comment
		var sidecar_source = Comment.get_comment_file (source_file);
		yield copy_file_to_destination (sidecar_source,
			Comment.get_comment_file (destination_file),
			flags | CopyFlags.MAKE_DESTINATION | CopyFlags.OVERWRITE | CopyFlags.IGNORE_ERRORS,
			job);
		copied_sources.add (sidecar_source);

		// XMP sidecar
		sidecar_source = Util.get_xmp_sidecar (source_file);
		yield copy_file_to_destination (sidecar_source,
			Util.get_xmp_sidecar (destination_file),
			flags | CopyFlags.OVERWRITE | CopyFlags.IGNORE_ERRORS,
			job);
		copied_sources.add (sidecar_source);
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
				copy_flags |= FileCopyFlags.ALL_METADATA | FileCopyFlags.TARGET_DEFAULT_MODIFIED_TIME;
			}
			if ((CopyFlags.MOVE in flags) || (CopyFlags.RENAME in flags)) {
				yield source_file.move_async (destination_file,
					copy_flags,
					Priority.DEFAULT,
					job.cancellable,
					(file_current_bytes, file_total_bytes) => update_file_progress (job, flags, file_current_bytes)
				);
			}
			else {
				yield source_file.copy_async (destination_file,
					copy_flags,
					Priority.DEFAULT,
					job.cancellable,
					(file_current_bytes, file_total_bytes) => update_file_progress (job, flags, file_current_bytes)
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

	void update_file_progress (Job job, CopyFlags flags, int64 file_current_bytes) {
		if (CopyFlags.UPDATE_PROGRESS in flags) {
			job.progress = Util.calc_progress (copied_bytes + (uint64) file_current_bytes, total_bytes);
		}
	}

	weak Window window;
	File last_made_destination;
	uint total_files;
	uint current_file;
	uint64 total_bytes;
	uint64 copied_bytes;
	OverwriteResponse last_overwrite_response;
	GenericSet<File> copied_sources;
}


[Flags]
public enum Gth.CopyFlags {
	DEFAULT,
	MAKE_DESTINATION,
	OVERWRITE,
	IGNORE_ERRORS,
	MOVE,
	DUPLICATE,
	RENAME,
	ALL_METADATA,
	UPDATE_PROGRESS,
}


class Gth.DeleteOperation {
	public DeleteOperation (Window _window) {
		window = _window;
	}

	public async void delete_files_from_disk (GenericList<File> files, Job job) throws Error {
		var file_data_list = yield FileManager.query_list_info (files, REQUIRED_ATTRIBUTES, QueryListFlags.DEFAULT, job.cancellable);
		var deleted_files = new GenericList<File>();
		var deleted_directories = new GenericList<File>();
		try {
			var iter = file_data_list.iterator ();
			while (iter.next ()) {
				var file_data = iter.get ();
				//stdout.printf ("> DELETE %s\n", file_data.file.get_uri ());
				job.progress = Util.calc_progress (iter.index (), files.model.n_items);
				job.subtitle = file_data.info.get_display_name ();
				switch (file_data.info.get_file_type ()) {
				case FileType.DIRECTORY:
					deleted_directories.model.append (file_data.file);
					deleted_files.model.append (file_data.file);
					break;

				case FileType.REGULAR:
					try {
						yield delete_file (file_data.file, job.cancellable);
					}
					catch (Error error) {
						// Ignore NOT_FOUND errors.
						// These are sidecar files already deleted in delete_file()
						if (!(error is IOError.NOT_FOUND)) {
							throw error;
						}
					}
					deleted_files.model.append (file_data.file);
					break;

				default:
					break;
				}
			}
		}
		finally {
			// Remove the directories.
			var iter = deleted_directories.iterator ();
			while (iter.previous ()) {
				var dir = iter.get ();
				try {
					yield dir.delete_async (Priority.DEFAULT, job.cancellable);
				}
				catch (Error error) {
					// Ignore errors
					//stdout.printf ("> ERROR: %s\n", error.message);
				}
			}
			app.monitor.files_deleted_from_disk (deleted_files);
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
