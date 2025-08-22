public class Gth.FileSourceCatalogs : Gth.FileSource {
	public override bool supports_scheme (string uri) {
		return uri.has_prefix ("catalog:///");
	}

	public override async GenericArray<FileData>? get_roots (Cancellable cancellable) {
		var roots = new GenericArray<FileData>();
		var file = File.new_for_uri ("catalog:///");
		var info = new FileInfo ();
		Catalog.update_file_info_for_library (file, info);
		roots.add (new Gth.FileData (file, info));
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

		var gio_source = new FileSourceVfs ();
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
					(ForEachFlags.FOLLOW_LINKS in flags) ? FileQueryInfoFlags.NONE : FileQueryInfoFlags.NOFOLLOW_SYMLINKS,
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
					var gio_folder = Catalog.to_gio_file (folder_data.file);
					var data = yield Files.load_contents_async (gio_folder, cancellable);
					var catalog = Catalog.new_from_data (folder_data.file, data);
					foreach (var file in catalog.files) {
						try {
							var file_data = yield gio_source.read_metadata (file, all_attributes, cancellable);
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

	public override void monitor_directory (File file, bool activate) {
		// TODO
	}

	public override async void copy_files (Window window, GenericList<File> files, File destination, Job job) throws Error {
		// TODO: add to catalog
	}
}
