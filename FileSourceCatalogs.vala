public class Gth.FileSourceCatalogs : Gth.FileSource {
	public override bool supports_scheme (string uri) {
		return uri.has_prefix ("catalog:///");
	}

	public override async GenericArray<FileData>? get_roots (Cancellable cancellable) {
		var roots = new GenericArray<FileData>();
		var file = File.new_for_uri ("catalog:///");
		var info = get_display_info (file);
		roots.add (new Gth.FileData (file, info));
		return roots;
	}

	public override FileInfo get_display_info (File file) {
		FileInfo info = null;
		var gio_file = Catalog.to_gio_file (file);
		var uri = file.get_uri ();
		if (uri.has_suffix (".catalog") || uri.has_suffix (".search")) {
			try {
				var data = Files.load_contents (gio_file, null);
				var catalog = Catalog.new_from_data (data);
				info = new FileInfo ();
				catalog.update_file_info (file, info);
			}
			catch (Error error) {
				stdout.printf ("ERROR: %s\n", error.message);
			}
		}
		else {
			info = gio_file.query_info (STANDARD_ATTRIBUTES, FileQueryInfoFlags.NONE);
			if (info.get_file_type () == FileType.DIRECTORY) {
				Catalog.update_file_info_for_library (file, info);
			}
		}
		if (info == null)
			info = new FileInfo ();
		return info;
	}

	public override async void foreach_child (
		File parent,
		ForEachFlags flags,
		string? attributes,
		Cancellable cancellable,
		ForEachChildFunc child_func) throws Error
	{
		var gio_file = Catalog.to_gio_file (parent);
		var info = yield gio_file.query_info_async (
			FileAttribute.STANDARD_TYPE,
			FileQueryInfoFlags.NONE,
			Priority.DEFAULT,
			cancellable);

		var queue = new Queue<FileData>();
		queue.push_tail (new Gth.FileData (parent, info));

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
					cancellable
				);
				while ((info = enumerator.next_file (cancellable)) != null) {
					if (info.get_is_hidden ()) {
						continue;
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
							var catalog = Catalog.new_from_data (data);
							catalog.update_file_info (child_data.file, child_data.info);
							action = child_func (child_data, false);
						}
						catch (Error error) {
							//stdout.printf ("ERROR: %s\n", error.message);
						}
						break;

					default:
						break;
					}
					if (action != ForEachAction.CONTINUE) {
						break;
					}
				}
				break;

			case FileType.REGULAR:
				try {
					var data = yield Files.load_contents_async (gio_file, cancellable);
					var catalog = Catalog.new_from_data (data);
					foreach (var file in catalog.files) {
						try {
							var file_data = yield gio_source.read_metadata (file, attributes, cancellable);
							action = child_func (file_data, false);
						}
						catch (Error error) {
							stdout.printf ("ERROR [1]: %s\n", error.message);
						}
						if (action == ForEachAction.STOP) {
							break;
						}
					}
				}
				catch (Error error) {
					stdout.printf ("ERROR [2]: %s\n", error.message);
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

	public override async Gth.FileData read_metadata (
		File file,
		string? requested_attributes,
		Cancellable cancellable) throws Error
	{
		var gio_file = Catalog.to_gio_file (file);
		var all_attributes = Util.concat_attributes (FileAttribute.STANDARD_TYPE, requested_attributes);
		var info = yield gio_file.query_info_async (all_attributes, FileQueryInfoFlags.NONE, Priority.DEFAULT, cancellable);
		if (info.get_file_type () == FileType.DIRECTORY) {
			Catalog.update_file_info_for_library (file, info);
		}
		else if (info.get_file_type () == FileType.REGULAR) {
			var data = yield Files.load_contents_async (gio_file, cancellable);
			var catalog = Catalog.new_from_data (data);
			catalog.update_file_info (file, info);
		}
		else {
			throw new IOError.FAILED ("Wrong file type");
		}
		return new Gth.FileData (file, info);
	}

	public override void monitor_directory (File file, bool activate) {
		// TODO
	}
}
