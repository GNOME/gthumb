public class Gth.FileSourceVfs : FileSource {
	public override bool supports_scheme (string uri) {
		// Always try to use GIO to load locations.
		return true;
	}

	public override async GenericList<FileData>? get_roots (Cancellable cancellable) {
		var roots = new GenericList<FileData>();
		yield add_root (roots, Files.get_home (), cancellable);
		yield add_root (roots, Files.get_special_dir (UserDirectory.PICTURES), cancellable);
		yield add_root (roots, Files.get_special_dir (UserDirectory.VIDEOS), cancellable);
		yield add_root (roots, Files.get_special_dir (UserDirectory.DOWNLOAD), cancellable);
		yield add_root (roots, File.new_for_uri ("file:///"), cancellable);
		FileSourceVfs.add_mountable_volumes (roots);
		return roots;
	}

	public static void add_mountable_volumes (GenericList<FileData> roots) {
		var monitor = VolumeMonitor.get ();
		var mounts = monitor.get_mounts ();
		foreach (var mount in mounts) {
			if (mount.is_shadowed ()) {
				continue;
			}
			var file = mount.get_root ();
			if (file_is_present (roots, file)) {
				continue;
			}
			var info = new FileInfo ();
			info.set_file_type (FileType.DIRECTORY);
			info.set_attribute_boolean (FileAttribute.ACCESS_CAN_READ, true);
			info.set_attribute_boolean (FileAttribute.ACCESS_CAN_WRITE, false);
			info.set_attribute_boolean (FileAttribute.ACCESS_CAN_DELETE, false);
			info.set_attribute_boolean (FileAttribute.ACCESS_CAN_TRASH, false);
			info.set_symbolic_icon (mount.get_symbolic_icon ());
			var name = mount.get_name ();
			var drive = mount.get_drive ();
			if (drive != null) {
				name = "%s: %s".printf (drive.get_name (), name);
			}
			info.set_display_name (name);
			info.set_name (name);
			roots.model.append (new Gth.FileData (file, info));
		}

		// Not mounted mountable volumes.
		var volumes = monitor.get_volumes ();
		foreach (var volume in volumes) {
			var mount = volume.get_mount ();
			if (mount != null) {
				// Already mounted, ignore.
				continue;
			}

			var file = volume.get_activation_root ();
			if (file == null) {
				var device = volume.get_identifier (VolumeIdentifier.UNIX_DEVICE);
				if (device == null)
					continue;
				file = File.new_for_path (device);
			}

			if (file_is_present (roots, file)) {
				// Already mounted, ignore.
				continue;
			}

			var info = new FileInfo ();
			info.set_file_type (FileType.MOUNTABLE);
			info.set_attribute_object (VOLUME_ATTRIBUTE, volume);
			info.set_attribute_boolean (FileAttribute.ACCESS_CAN_READ, true);
			info.set_attribute_boolean (FileAttribute.ACCESS_CAN_WRITE, false);
			info.set_attribute_boolean (FileAttribute.ACCESS_CAN_DELETE, false);
			info.set_attribute_boolean (FileAttribute.ACCESS_CAN_TRASH, false);
			info.set_symbolic_icon (volume.get_symbolic_icon ());
			info.set_display_name (volume.get_name ());
			info.set_name (volume.get_name ());
			roots.model.append (new Gth.FileData (file, info));
		}
	}

	public override FileInfo get_display_info (File file) {
		FileInfo info = null;
		string icon_name = null;
		var uri = file.get_uri ();
		if (uri.has_prefix ("file://")) {
			try {
				info = file.query_info (
					(FileAttribute.STANDARD_TYPE + "," +
					 FileAttribute.STANDARD_DISPLAY_NAME + "," +
					 FileAttribute.STANDARD_SYMBOLIC_ICON),
					FileQueryInfoFlags.NONE,
					null);
					icon_name = (info.get_file_type () == FileType.DIRECTORY) ? "folder-symbolic" : "gth-file-symbolic";
			}
			catch (Error error) {
				icon_name = "gth-file-symbolic";
			}
		}
		if (icon_name == null) {
			icon_name = "folder-remote-symbolic";
		}
		if (info == null) {
			info = new FileInfo ();
			var name = file.get_basename ();
			if (name == null)
				name = file.get_uri ();
			info.set_display_name (name);
		}
		if (!info.has_attribute (FileAttribute.STANDARD_SYMBOLIC_ICON)) {
			info.set_symbolic_icon (Util.get_themed_icon (icon_name));
		}
		Files.update_special_location_info (file, info);
		return info;
	}

	public override async void foreach_child (File parent, ForEachFlags flags, string attributes, Cancellable cancellable, ForEachChildFunc child_func) throws Error {
		flags |= ForEachFlags.READ_METADATA;
		yield FileManager.foreach_child (parent, flags, attributes, cancellable, child_func);
	}

	public override async Gth.FileData read_metadata (File file, string requested_attributes, Cancellable cancellable) throws Error	{
		return yield FileManager.read_metadata (file, requested_attributes, cancellable);
	}

	public override void monitor_directory (File file, bool activate) {
		app.events.watch_file (file, activate);
	}

	public override async void add_files (Window window, File destination, GenericList<File> files, Job job) throws Error {
		yield window.file_manager.copy_files (files, destination, job);
	}

	public override async void remove_files (Window window, File location, GenericList<File> files, Job job) throws Error {
		yield window.file_manager.trash_files (files, job);
	}

	const string ROOT_ATTRIBUTES =
		FileAttribute.STANDARD_TYPE + "," +
		FileAttribute.STANDARD_NAME + "," +
		FileAttribute.STANDARD_DISPLAY_NAME + "," +
		FileAttribute.STANDARD_SYMBOLIC_ICON + "," +
		ACCESS_ATTRIBUTES;

	async void add_root (GenericList<FileData> roots, File file, Cancellable cancellable) {
		if ((file == null) || file_is_present (roots, file)) {
			return;
		}
		try {
			var file_data = yield read_metadata (file, ROOT_ATTRIBUTES, cancellable);
			roots.model.append (file_data);
		}
		catch (Error error) {
			// Ignore.
		}
	}

	static bool file_is_present (GenericList<FileData> roots, File file) {
		foreach (unowned var root in roots) {
			if (root.file.equal (file))
				return true;
		}
		return false;
	}
}
