public class Gth.FileSourceVfs : FileSource {
	public override bool supports_scheme (string uri) {
		// Always try to use GIO to load locations.
		return true;
	}

	public override async GenericArray<FileData>? get_roots (Cancellable cancellable) {
		var roots = new GenericArray<FileData>();
		yield add_root (roots, Files.get_home (), cancellable);
		yield add_root (roots, Files.get_special_dir (UserDirectory.PICTURES), cancellable);
		yield add_root (roots, Files.get_special_dir (UserDirectory.VIDEOS), cancellable);
		yield add_root (roots, Files.get_special_dir (UserDirectory.DOWNLOAD), cancellable);
		yield add_root (roots, File.new_for_uri ("file:///"), cancellable);

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
			roots.add (new Gth.FileData (file, info));
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
			roots.add (new Gth.FileData (file, info));
		}

		return roots;
	}

	public override FileInfo get_display_info (File file) {
		FileInfo info = null;
		string icon_name = null;
		var uri = file.get_uri ();
		if (uri.has_prefix ("file://")) {
			try {
				info = file.query_info (
					FileAttribute.STANDARD_DISPLAY_NAME + "," + FileAttribute.STANDARD_SYMBOLIC_ICON,
					FileQueryInfoFlags.NONE,
					null);
			}
			catch (Error error) {
			}
			icon_name = "folder-symbolic";
		}
		else {
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
		update_special_location_info (file, info);
		return info;
	}

	void update_special_location_info (File file, FileInfo info) {
		if (file.equal (Files.get_root ())) {
			info.set_display_name (_("Computer"));
			info.set_symbolic_icon (new ThemedIcon ("drive-harddisk-symbolic"));
		}
		else if (file.equal (Files.get_home ())) {
			info.set_display_name (_("User Home"));
		}
	}

	const int REMOTE_FILES_PER_REQUEST = 100;
	const int LOCAL_FILES_PER_REQUEST = 1000;

	public override async void foreach_child (File parent, ForEachFlags flags, string attributes, Cancellable cancellable, ForEachChildFunc child_func) throws Error {
		var all_attributes = Util.concat_attributes (REQUIRED_ATTRIBUTES, attributes);
		var metadata_attributes_v = Util.extract_metadata_attributes (all_attributes);
		var has_symbolic_icon = Util.attributes_match_all_patterns (FileAttribute.STANDARD_SYMBOLIC_ICON, all_attributes);
		var file_attributes = Util.extract_file_attributes (all_attributes);
		var parent_info = yield parent.query_info_async (file_attributes, FileQueryInfoFlags.NONE, Priority.DEFAULT, cancellable);
		if (parent_info.get_file_type () != FileType.DIRECTORY)
			throw new IOError.FAILED ("Not a directory");

		var queue = new Queue<FileData>();
		queue.push_tail (new Gth.FileData (parent, parent_info));

		var is_local = parent.get_uri_scheme () == "file";
		var files_per_request = is_local ? LOCAL_FILES_PER_REQUEST : REMOTE_FILES_PER_REQUEST;

		while (queue.length > 0) {
			var folder_data = queue.pop_head ();
			var action = child_func (folder_data, true);
			if (action == ForEachAction.SKIP) {
				continue;
			}
			if (action == ForEachAction.STOP) {
				break;
			}
			var enumerator = yield folder_data.file.enumerate_children_async (
				file_attributes,
				(ForEachFlags.FOLLOW_LINKS in flags) ? FileQueryInfoFlags.NONE : FileQueryInfoFlags.NOFOLLOW_SYMLINKS,
				Priority.DEFAULT,
				cancellable);
			while (action != ForEachAction.STOP) {
				var info_list = yield enumerator.next_files_async (files_per_request, Priority.DEFAULT, cancellable);
				if (info_list == null)
					break;
				foreach (var info in info_list) {
					var child = enumerator.get_child (info);
					var child_data = new Gth.FileData (child, info);

					if (!has_symbolic_icon && (info.get_file_type () == FileType.DIRECTORY)) {
						// Always set the symbolic icon for directories.
						var more_info = yield child.query_info_async (
							FileAttribute.STANDARD_SYMBOLIC_ICON,
							FileQueryInfoFlags.NONE,
							Priority.DEFAULT,
							cancellable);
						if (more_info.has_attribute (FileAttribute.STANDARD_SYMBOLIC_ICON)) {
							info.set_symbolic_icon (more_info.get_symbolic_icon ());
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
					else if ((info.get_file_type () == FileType.REGULAR)
						&& (metadata_attributes_v.length > 0))
					{
						read_metadata_attributes (child_data, metadata_attributes_v, cancellable);
					}
					if (cancellable.is_cancelled ()) {
						action = ForEachAction.STOP;
						break;
					}
				}
			}
			if (action == ForEachAction.STOP) {
				break;
			}
		}
	}

	public override async Gth.FileData read_metadata (File file, string requested_attributes, Cancellable cancellable) throws Error	{
		var all_attributes = Util.concat_attributes (REQUIRED_ATTRIBUTES, requested_attributes);
		var file_attributes = Util.extract_file_attributes (all_attributes);
		var info = yield file.query_info_async (file_attributes, FileQueryInfoFlags.NONE, Priority.DEFAULT, cancellable);
		update_special_location_info (file, info);
		var file_data = new Gth.FileData (file, info);
		var metadata_attributes_v = Util.extract_metadata_attributes (all_attributes);
		read_metadata_attributes (file_data, metadata_attributes_v, cancellable);
		return file_data;
	}

	public override void monitor_directory (File file, bool activate) {
		// TODO
	}

	public override async void copy_files (Window window, GenericList<File> files, File destination, Job job) throws Error {
		yield window.file_manager.copy_files (files, destination, job);
	}

	void read_metadata_attributes (FileData file_data, string[] metadata_attributes_v, Cancellable cancellable) {
		if (metadata_attributes_v.length > 0) {
			foreach (unowned var provider in app.metadata_providers) {
				if (provider.can_read (file_data, file_data.get_content_type (), metadata_attributes_v)) {
					provider.read (file_data, metadata_attributes_v, cancellable);
				}
			}
		}
	}

	const string ROOT_ATTRIBUTES =
		FileAttribute.STANDARD_TYPE + "," +
		FileAttribute.STANDARD_NAME + "," +
		FileAttribute.STANDARD_DISPLAY_NAME + "," +
		FileAttribute.STANDARD_SYMBOLIC_ICON + "," +
		ACCESS_ATTRIBUTES;

	async void add_root (GenericArray<FileData> roots, File file, Cancellable cancellable) {
		if ((file == null) || file_is_present (roots, file)) {
			return;
		}
		try {
			var file_data = yield read_metadata (file, ROOT_ATTRIBUTES, cancellable);
			roots.add (file_data);
		}
		catch (Error error) {
			// Ignore.
		}
	}

	bool file_is_present (GenericArray<FileData> roots, File file) {
		foreach (unowned var root in roots) {
			if (root.file.equal (file))
				return true;
		}
		return false;
	}
}
