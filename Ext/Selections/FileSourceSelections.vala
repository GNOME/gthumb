public class Gth.FileSourceSelections : Gth.FileSource {
	public override bool supports_scheme (string uri) {
		return uri.has_prefix ("selection:///");
	}

	public override async GenericList<FileData>? get_roots (Cancellable cancellable) {
		var roots = new GenericList<FileData>();
		roots.model.append (Selection.get_root ());
		return roots;
	}

	public override FileInfo get_display_info (File file) {
		var info = new FileInfo();
		Selection.update_file_info (file, info);
		return info;
	}

	public override async void foreach_child (File parent, ForEachFlags flags, string attributes, Cancellable cancellable, ForEachChildFunc child_func) throws Error {
		var all_attributes = Util.concat_attributes (REQUIRED_ATTRIBUTES, attributes);

		var parent_info = new FileInfo ();
		Selection.update_file_info (parent, parent_info);
		var parent_data = new Gth.FileData (parent, parent_info);

		var queue = new Queue<FileData>();
		queue.push_tail (parent_data);

		while (queue.length > 0) {
			var folder_data = queue.pop_head ();
			//stdout.printf ("> FOLDER DATA: %s\n", folder_data.file.get_uri ());
			var action = child_func (folder_data, true);
			if (action == ForEachAction.SKIP) {
				//stdout.printf ("  SKIP\n");
				continue;
			}
			if (action == ForEachAction.STOP) {
				//stdout.printf ("  STOP\n");
				break;
			}
			if (folder_data.info.get_file_type () != FileType.DIRECTORY) {
				continue;
			}

			var number = Selection.get_number (folder_data.file);
			if (number == 0) {
				for (var i = 1; i <= Selection.MAX_SELECTIONS; i++) {
					var child_data = Selection.get_selection_data ((uint) i);
					if (ForEachFlags.RECURSIVE in flags) {
						queue.push_tail (child_data);
					}
					else {
						action = child_func (child_data, false);
					}
					if (action != ForEachAction.CONTINUE) {
						break;
					}
				}
			}
			else {
				var selection_changed = false;
				try {
					var selection = app.selections.get_selection (number);
					if (selection == null) {
						throw new IOError.FAILED ("Wrong Selection");
					}
					uint position = 0;
					var local_files = selection.get_files ();
					foreach (var file in local_files) {
						try {
							var file_data = yield FileManager.read_metadata (file, all_attributes, cancellable);
							file_data.set_position (position);
							action = child_func (file_data, false);
						}
						catch (Error error) {
							//stdout.printf ("ERROR [1]: %s\n", error.message);
							if (cancellable.is_cancelled ()) {
								action = ForEachAction.STOP;
							}
							else {
								selection.remove_file (file);
								selection_changed = true;
							}
						}
						if (action == ForEachAction.STOP) {
							break;
						}
						position++;
					}
				}
				catch (Error error) {
					//stdout.printf ("ERROR [2]: %s\n", error.message);
					if (cancellable.is_cancelled ()) {
						action = ForEachAction.STOP;
					}
				}
				finally {
					if (selection_changed) {
						app.monitor.selection_changed (number);
					}
				}
			}
			if (action == ForEachAction.STOP) {
				break;
			}
		}
	}

	public override async Gth.FileData read_metadata (File file, string requested_attributes, Cancellable cancellable) throws Error {
		var info = new FileInfo ();
		Selection.update_file_info (file, info);
		return new Gth.FileData (file, info);
	}

	public override async void add_files (Window window, File destination, GenericList<File> files, Job job) throws Error {
		var selection = app.selections.get_selection_from_file (destination);
		if (selection == null) {
			throw new IOError.FAILED ("Wrong Destination");
		}
		selection.add_files (files);
	}

	public override async void remove_files (Window window, File location, GenericList<File> files, Job job) throws Error {
		var selection = app.selections.get_selection_from_file (location);
		if (selection == null) {
			throw new IOError.FAILED ("Wrong Destination");
		}
		selection.remove_files (files);
	}

	public override async void save_order (Window window, File location, GenericList<File> files, Job job) throws Error {
		var selection = app.selections.get_selection_from_file (location);
		if (selection == null) {
			throw new IOError.FAILED ("Wrong Destination");
		}
		selection.set_files (files);
	}

	public override bool is_reorderable () {
		return true;
	}

	public override async void files_renamed (Window window, File location, GenericList<RenamedFile> renamed_files, Job job) throws Error {
		var selection = app.selections.get_selection_from_file (location);
		if (selection == null) {
			throw new IOError.FAILED ("Wrong Destination");
		}
		selection.files_renamed (renamed_files);
	}
}
