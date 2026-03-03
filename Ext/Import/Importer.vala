public class Gth.Importer {
	public File destination_folder;
	public string subfolder;
	public bool automatic_subfolder;
	public AutomaticDate automatic_subfolder_date;
	public string automatic_subfolder_format;
	public bool delete_imported;
	public GenericArray<FileData> files;

	public async void import_files (Gth.Window window, Gth.Job job) throws Error {
		job.subtitle = null;
		var total_files = files.length;
		var created_subfolders = new GenericSet<string> (str_hash, str_equal);
		var today = new GLib.DateTime.now_local ();
		var last_overwrite_response = OverwriteResponse.NONE;
		var copy_operation = new CopyOperation (window);
		copy_operation.total_files = total_files;
		var delete_operation = new DeleteOperation (window);
		var save_operation = new SaveOperation (window);
		save_operation.total_files = total_files;
		var current_file = 0;
		var catalogs = new HashTable<string, Catalog> (str_hash, str_equal);
		foreach (var file_data in files) {
			job.progress = Util.calc_progress (current_file++, total_files);
			Bytes bytes = null;
			var relative_path = new GenericArray<string?>();
			if (!Strings.empty (subfolder)) {
				relative_path.add (subfolder);
			}
			GLib.DateTime datetime = today;
			if (automatic_subfolder) {
				switch (automatic_subfolder_date) {
				case AutomaticDate.MODIFIED:
					datetime = file_data.info.get_modification_date_time ();
					break;

				case AutomaticDate.ORIGINAL:
					// Read the embedded date.
					string[] attributes_v = { "Metadata::DateTime" };
					var metadata_provider = new ExivMetadataProvider ();
					if (metadata_provider.can_read (file_data.file, file_data.info, attributes_v)) {
						bytes = yield Files.load_file_async (file_data.file, job.cancellable);
						metadata_provider.read (file_data.file, bytes, file_data.info, job.cancellable);
					}
					if (file_data.info.has_attribute ("Metadata::DateTime")) {
						var metadata = file_data.info.get_attribute_object ("Metadata::DateTime") as Metadata;
						var metadata_datetime = Gth.DateTime.get_from_exif_date (metadata.raw);
						if (metadata_datetime != null) {
							var original_datetime = metadata_datetime.date.to_local_gdatetime ();
							if (original_datetime != null) {
								datetime = original_datetime;
							}
						}
					}
					break;

				default:
					// datetime = today;
					break;
				}
				relative_path.add (datetime.format (automatic_subfolder_format));
			}
			if (relative_path.length > 0) {
				var relative_path_s = string.joinv ("/", relative_path.data);
				if (!created_subfolders.contains (relative_path_s)) {
					var relative_path_v = relative_path_s.split ("/");
					var directory = destination_folder;
					foreach (unowned var child in relative_path_v) {
						directory = directory.get_child (child);
						yield Files.make_directory_async (directory, job.cancellable);
					}
					created_subfolders.add (relative_path_s);
				}
			}
			relative_path.add (file_data.file.get_basename ());
			relative_path.add (null); // for string.joinv
			var destination = destination_folder.get_child (string.joinv ("/", relative_path.data));
			if (destination.equal (file_data.file)) {
				continue;
			}

			if (bytes != null) {
				stdout.printf ("> SAVE: %s -> %s\n", file_data.file.get_uri (), destination.get_uri ());
				save_operation.last_overwrite_response = last_overwrite_response;
				yield save_operation.save (destination, bytes, job);
				last_overwrite_response = save_operation.last_overwrite_response;
			}
			else {
				stdout.printf ("> COPY: %s -> %s\n", file_data.file.get_uri (), destination.get_uri ());
				copy_operation.last_overwrite_response = last_overwrite_response;
				yield copy_operation.copy_file (file_data.file, destination, CopyFlags.DEFAULT, job);
				last_overwrite_response = copy_operation.last_overwrite_response;
			}

			// Catalog by date
			var catalog_id = datetime.format ("%Y-%m-%d");
			var catalog = catalogs.get (catalog_id);
			if (catalog == null) {
				var catalog_file = Catalog.from_gdatetime (datetime, ".catalog");
				try {
					catalog = yield Catalog.load_from_file (catalog_file, job.cancellable);
				}
				catch (Error error) {
					catalog = new Catalog ();
					catalog.file = catalog_file;
					catalog.date = Date.from_gdatetime (datetime);
					catalogs.set (catalog_id, catalog);
				}
			}
			catalog.add_file (destination);

			// "Last Imported" catalog
			catalog = catalogs.get (LAST_IMPORTED_KEY);
			if (catalog == null) {
				if (subfolder != null) {
					// Append files to the catalog if an event name was given.
					var catalog_file = Catalog.from_display_name (subfolder, ".catalog");
					try {
						catalog = yield Catalog.load_from_file (catalog_file, job.cancellable);
					}
					catch (Error error) {
						catalog = new Catalog ();
						catalog.file = catalog_file;
						catalog.date = Date.from_gdatetime (today);
						catalogs.set (LAST_IMPORTED_KEY, catalog);
					}
				}
				else {
					// Overwrite the catalog content if the generic "last imported" catalog is used.
					var catalog_file = Catalog.from_display_name (_("Last Imported"), ".catalog");
					catalog = new Catalog ();
					catalog.file = catalog_file;
					catalog.date = Date.from_gdatetime (today);
					catalogs.set (LAST_IMPORTED_KEY, catalog);
				}
			}
			catalog.add_file (destination);

			if (delete_imported) {
				try {
					yield delete_operation.delete_file (file_data.file, job.cancellable);
				}
				catch (Error error) {
				}
			}
		}

		var created_directories = new GenericSet<File> (Util.file_hash, Util.file_equal);
		foreach (Catalog catalog in catalogs.get_values ()) {
			var gio_file = Catalog.to_gio_file (catalog.file);
			var gio_dir = gio_file.get_parent ();
			if (!created_directories.contains (gio_dir)) {
				Files.ensure_directory_exists (gio_dir);
				created_directories.add (gio_dir);
			}
			yield catalog.save_async (job.cancellable);
		}
	}

	const string LAST_IMPORTED_KEY = "last-imported";
}

public class Gth.SaveOperation {
	public GenericList<File> saved_files;
	public OverwriteResponse last_overwrite_response;
	public uint total_files;

	public SaveOperation (Window _window) {
		window = _window;
		saved_files = new GenericList<File>();
		last_overwrite_response = OverwriteResponse.NONE;
		total_files = 1;
	}

	public async void save (File destination, Bytes bytes, Job job) throws Error {
		if (last_overwrite_response == OverwriteResponse.OVERWRITE_ALL) {
			yield replace (destination, bytes, job);
		}
		else {
			yield create (destination, bytes, job);
		}
	}

	async void replace (File destination, Bytes bytes, Job job) throws Error {
		yield Files.save_file_async (destination, bytes, job.cancellable);
		saved_files.model.append (destination);
	}

	async void create (File destination, Bytes bytes, Job job, Image? image = null) throws Error {
		try {
			yield Files.create_file_async (destination, bytes, job.cancellable);
			saved_files.model.append (destination);
			return;
		}
		catch (Error error) {
			if (!(error is IOError.EXISTS)) {
				throw error;
			}
		}

		// error is IOError.EXISTS

		if (last_overwrite_response == OverwriteResponse.SKIP_ALL) {
			return;
		}

		if (image == null) {
			try {
				image = yield app.image_loader.load_bytes (window.monitor_profile,
					bytes, null, LoadFlags.NO_METADATA, job.cancellable);
			}
			catch (Error error) {
			}
		}

		var overwrite = new OverwriteDialog (window);
		overwrite.single_file = total_files == 1;
		last_overwrite_response = yield overwrite.ask_image (image, destination, OverwriteRequest.FILE_EXISTS, job);
		switch (last_overwrite_response) {
		case OverwriteResponse.CANCEL:
			throw new IOError.CANCELLED ("Cancelled");

		case OverwriteResponse.OVERWRITE, OverwriteResponse.OVERWRITE_ALL:
			yield replace (destination, bytes, job);
			break;

		case OverwriteResponse.RENAME:
			var new_destination = destination.get_parent ().get_child_for_display_name (overwrite.new_name);
			yield create (new_destination, bytes, job, image);
			break;

		default:
			break;
		}
	}
	weak Window window;
}
