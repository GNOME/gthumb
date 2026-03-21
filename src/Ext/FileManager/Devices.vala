public class Gth.Devices {
	public Devices () {
		requests = new Queue<Callback>();
		mounting = false;
	}

	// Mount the location if required.
	public async FileData? ensure_mounted (File location, Gtk.Window window, Cancellable? cancellable = null) throws Error {
		yield app.update_roots ();
		var nearest_root = get_nearest_root (location);
		if ((nearest_root != null) && (nearest_root.info.get_file_type () == FileType.DIRECTORY)) {
			return nearest_root;
		}

		if (mounting) {
			requests.push_tail (new Callback (ensure_mounted.callback));
			yield;
			nearest_root = get_nearest_root (location);
			if ((nearest_root != null) && (nearest_root.info.get_file_type () == FileType.DIRECTORY)) {
				return nearest_root;
			}
		}

		try {
			mounting = true;
			var try_again = false;
			if (nearest_root == null) {
				// Mount the enclosing volume.
				var mount_op = new Gtk.MountOperation (window);
				yield location.mount_enclosing_volume (0, mount_op, cancellable);
				try_again = true;
			}
			else if (nearest_root.info.get_file_type () == FileType.MOUNTABLE) {
				var volume = nearest_root.info.get_attribute_object (PrivateAttribute.VOLUME) as Volume;
				if (volume != null) {
					var mount_op = new Gtk.MountOperation (window);
					yield volume.mount (0, mount_op, cancellable);
					var mount = volume.get_mount ();
					if (mount == null) {
						throw new IOError.NOT_MOUNTED ("Location not available");
					}
				}
				else {
					var mount_op = new Gtk.MountOperation (window);
					yield nearest_root.file.mount_mountable (0, mount_op, cancellable);
				}
				try_again = true;
			}
			if (try_again) {
				yield app.update_roots ();
				nearest_root = get_nearest_root (location);
				if ((nearest_root == null) || (nearest_root.info.get_file_type () != FileType.DIRECTORY)) {
					throw new IOError.NOT_MOUNTED ("Location not available");
				}
			}
		}
		catch (Error error) {
			throw error;
		}
		finally {
			mounting = false;
			if (!requests.is_empty ()) {
				var first_callback = requests.pop_head ();
				Idle.add ((owned) first_callback.callback);
			}
		}
		return nearest_root;
	}

	FileData? get_nearest_root (File file) {
		FileData nearest_root = null;
		try {
			var file_uri = Uri.parse (file.get_uri (), UriFlags.PARSE_RELAXED);
			var file_path = file_uri.get_path ();
			var max_length = 0;
			foreach (unowned var root in app.roots) {
				var location_uri = Uri.parse (root.file.get_uri (), UriFlags.PARSE_RELAXED);
				if (location_uri.get_scheme () == file_uri.get_scheme ()) {
					var location_path = location_uri.get_path ();
					if (file_path.has_prefix (location_path)) {
						var len = location_path.length;
						if (len > max_length) {
							nearest_root = root;
							max_length = len;
						}
					}
				}
			}
		}
		catch (Error error) {
		}
		return nearest_root;
	}

	Queue<Callback> requests;
	bool mounting;
}

class Gth.Callback {
	public SourceFunc callback;

	public Callback (owned SourceFunc _callback) {
		callback = (owned) _callback;
	}
}
