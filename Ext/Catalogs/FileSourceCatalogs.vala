public class Gth.FileSourceCatalogs : Gth.FileSource {
	public override bool supports_scheme (string uri) {
		return uri.has_prefix ("catalog:///");
	}

	public override async GenericList<FileData>? get_roots (Cancellable cancellable) {
		var roots = new GenericList<FileData>();
		roots.model.append (Catalog.get_root ());
		return roots;
	}

	public override FileInfo get_display_info (File file) {
		var info = new FileInfo ();
		var gio_file = Catalog.to_gio_file (file);
		var uri = file.get_uri ();
		if (uri.has_suffix (".catalog") || uri.has_suffix (".search")) {
			try {
				var data = Files.load_contents (gio_file, null);
				var catalog = Catalog.new_from_data (file, data);
				catalog.update_file_info (info);
			}
			catch (Error error) {
				Catalog.update_file_info_for_broken_file (file, info);
			}
		}
		else {
			Catalog.update_file_info_for_library (file, info);
		}
		return info;
	}

	const int FILES_PER_REQUEST = 1000;

	public override async void foreach_child (File parent, ForEachFlags flags, string attributes, Cancellable cancellable, ForEachChildFunc child_func) throws Error {
		var all_attributes = Util.concat_attributes (REQUIRED_ATTRIBUTES, attributes);
		var file_attributes = Util.extract_file_attributes (all_attributes);
		var gio_file = Catalog.to_gio_file (parent);
		var parent_info = yield gio_file.query_info_async (
			file_attributes,
			FileQueryInfoFlags.NONE,
			Priority.DEFAULT,
			cancellable);

		var queue = new Queue<FileData>();
		queue.push_tail (new Gth.FileData (parent, parent_info));

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
			switch (folder_data.info.get_file_type ()) {
			case FileType.DIRECTORY:
				var gio_folder = Catalog.to_gio_file (folder_data.file);
				//stdout.printf ("  DIRECTORY %s\n", gio_folder.get_uri ());
				var enumerator = yield gio_folder.enumerate_children_async (
					STANDARD_ATTRIBUTES,
					(ForEachFlags.NOFOLLOW_LINKS in flags) ? FileQueryInfoFlags.NOFOLLOW_SYMLINKS : FileQueryInfoFlags.NONE,
					Priority.DEFAULT,
					cancellable);
				while (action != ForEachAction.STOP) {
					var info_list = yield enumerator.next_files_async (FILES_PER_REQUEST, Priority.DEFAULT, cancellable);
					if (info_list == null)
						break;
					foreach (var info in info_list) {
						if (info.get_is_hidden ()) {
							continue;
						}
						if (cancellable.is_cancelled ()) {
							action = ForEachAction.STOP;
							break;
						}
						var gio_child = enumerator.get_child (info);
						var child = Catalog.from_gio_file (gio_child);
						if (child == null) {
							continue;
						}
						//stdout.printf ("  CHILD: %s\n", child.get_uri ());
						var child_data = new Gth.FileData (child, info);
						switch (child_data.info.get_file_type ()) {
						case FileType.DIRECTORY:
							Catalog.update_file_info_for_library (child_data.file, child_data.info);
							if (ForEachFlags.RECURSIVE in flags) {
								queue.push_tail (child_data);
							}
							else {
								action = child_func (child_data, false);
							}
							break;

						case FileType.REGULAR:
							try {
								var data = yield Files.load_contents_async (gio_child, cancellable);
								var catalog = Catalog.new_from_data (child_data.file, data);
								catalog.update_file_info (child_data.info);
								action = child_func (child_data, false);
							}
							catch (Error error) {
								//stdout.printf ("ERROR: %s\n", error.message);
								if (cancellable.is_cancelled ()) {
									action = ForEachAction.STOP;
								}
							}
							break;

						default:
							break;
						}
						if (action != ForEachAction.CONTINUE) {
							break;
						}
					}
				}
				break;

			case FileType.REGULAR:
				try {
					var catalog = yield Catalog.load_from_file (folder_data.file, cancellable);
					uint position = 0;
					foreach (var file in catalog.files) {
						try {
							var file_data = yield FileData.read_metadata (file, all_attributes, cancellable);
							file_data.set_position (position);
							action = child_func (file_data, false);
						}
						catch (Error error) {
							//stdout.printf ("ERROR [1]: %s\n", error.message);
							if (cancellable.is_cancelled ()) {
								action = ForEachAction.STOP;
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
				break;

			default:
				break;
			}

			if (action == ForEachAction.STOP) {
				break;
			}
		}
	}

	public override async Gth.FileData read_metadata (File file, string requested_attributes, Cancellable cancellable) throws Error {
		var gio_file = Catalog.to_gio_file (file);
		var all_attributes = Util.concat_attributes (FileAttribute.STANDARD_TYPE, requested_attributes);
		var file_attributes = Util.extract_file_attributes (all_attributes);
		var info = yield gio_file.query_info_async (file_attributes, FileQueryInfoFlags.NONE, Priority.DEFAULT, cancellable);
		if (info.get_file_type () == FileType.DIRECTORY) {
			Catalog.update_file_info_for_library (file, info);
		}
		else if (info.get_file_type () == FileType.REGULAR) {
			var data = yield Files.load_contents_async (gio_file, cancellable);
			var catalog = Catalog.new_from_data (file, data);
			catalog.update_file_info (info);
		}
		else {
			throw new IOError.FAILED ("Wrong file type");
		}
		return new Gth.FileData (file, info);
	}

	public override async void add_files (Window window, File destination, GenericList<File> files, Job job) throws Error {
		yield Catalog.add_files (destination, files, job);
	}

	public override async void remove_files (Window window, File location, GenericList<File> files, Job job) throws Error {
		yield Catalog.remove_files (location, files, job);
	}

	public override async void save_order (Window window, File location, GenericList<File> files, Job job) throws Error {
		yield Catalog.save_order (location, files, job);
		// TODO app.monitor.order_changed (location);
	}

	public override bool is_reorderable () {
		return true;
	}

	public override async void files_renamed (Window window, File location, GenericList<RenamedFile> renamed_files, Job job) throws Error {
		yield Catalog.rename_files (location, renamed_files, job);
	}
}
