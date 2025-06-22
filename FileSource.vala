public abstract class Gth.FileSource : Object {
	public abstract bool supports_scheme (string uri);

	public abstract async GenericArray<FileData>? get_roots (Cancellable cancellable);

	public abstract FileInfo get_display_info (File file);

	public abstract async void foreach_child (
		File parent,
		ForEachFlags flags,
		string? attributes,
		Cancellable cancellable,
		ForEachChildFunc child_func) throws Error;

	public abstract async Gth.FileData read_metadata (
		File file,
		string? attributes,
		Cancellable cancellable) throws Error;

	public abstract void monitor_directory (File file, bool activate);

	public async GenericList<Gth.FileData> list_children (
		File parent,
		string? attributes,
		Cancellable cancellable) throws Error
	{
		//stdout.printf ("LIST CHILDREN %s: ATTRIBUTES: %s\n", parent.get_uri (), attributes);
		var list = new GenericList<Gth.FileData>();
		yield foreach_child (parent, ForEachFlags.FOLLOW_LINKS, attributes, cancellable, (file_data, is_parent) => {
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
}

[Flags]
public enum Gth.ForEachFlags {
	DEFAULT,
	RECURSIVE,
	FOLLOW_LINKS,
}

public enum Gth.ForEachAction {
	SKIP,
	CONTINUE,
	STOP;
}

public delegate Gth.ForEachAction Gth.ForEachChildFunc (Gth.FileData child, bool is_parent);
