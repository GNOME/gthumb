public abstract class Gth.FileSource : Object {
	public abstract bool supports_scheme (string uri);

	public abstract async GenericList<FileData>? get_roots (Cancellable cancellable);

	public abstract FileInfo get_display_info (File file);

	public abstract async Gth.FileData read_metadata (File file, string attributes, Cancellable cancellable) throws Error;

	public abstract async void foreach_child (File parent, ForEachFlags flags, string attributes, Cancellable cancellable, ForEachChildFunc child_func) throws Error;

	public async GenericList<Gth.FileData> list_children (File parent, string attributes, Cancellable cancellable) throws Error	{
		//stdout.printf ("LIST CHILDREN %s: ATTRIBUTES: %s\n", parent.get_uri (), attributes);
		var list = new GenericList<Gth.FileData>();
		yield foreach_child (parent, ForEachFlags.DEFAULT, attributes, cancellable, (file_data, is_parent) => {
			if (!is_parent) {
				switch (file_data.info.get_file_type ()) {
				case FileType.DIRECTORY, FileType.REGULAR:
					list.model.append (file_data);
					break;
				default:
					// Ignored
					break;
				}
			}
			return ForEachAction.CONTINUE;
		});
		return list;
	}

	public abstract void monitor_directory (File file, bool activate);

	// public abstract void monitor_roots (bool activate);

	public abstract async void add_files (Window window, File location, GenericList<File> files, Job job) throws Error;

	public abstract async void remove_files (Window window, File location, GenericList<File> files, Job job) throws Error;

	// public abstract async void deleted_from_disk (Window window, File location, GenericList<File> files, Job job) throws Error;

	// public abstract bool is_reorderable ();

	// public abstract async void reorder_files (File location, GenericList<File> visible_files, GenericList<File> files_to_move, int position) throws Error;

	// public abstract async uint64 get_free_space (File location);
}
