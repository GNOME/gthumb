public class Gth.FileSourceVfs : FileSource {
	public override bool supports_scheme (string uri) {
		// Always try to use GIO to load locations.
		return true;
	}

	const string REQUIRED_ATTRIBUTES = "standard::name,standard::type,standard::is-hidden,standard::is-backup,id::file";
	const string GIO_ATTRIBUTES = "standard::*,etag::*,id::*,access::*,mountable::*,time::*,unix::*,dos::*,owner::*,thumbnail::*,filesystem::*,gvfs::*,xattr::*,xattr-sys::*,selinux::*";

	public override async void foreach_child (
		File parent,
		ForEachFlags flags,
		string? attributes,
		Cancellable cancellable,
		ForEachChildFunc child_func) throws Error
	{
		var required_attributes = Util.concat_attributes (REQUIRED_ATTRIBUTES, attributes);
		var visited = new HashTable<string, string>(str_hash, str_equal);
		var metadata_attributes_v = Util.extract_metadata_attributes (required_attributes);

		var info = yield parent.query_info_async (attributes, FileQueryInfoFlags.NONE, Priority.DEFAULT, cancellable);
		if (info.get_file_type () != FileType.DIRECTORY)
			throw new IOError.FAILED ("Not a directory");

		var queue = new Queue<FileData>();
		queue.push_tail (new Gth.FileData (parent, info));

		while (queue.length > 0) {
			var folder_data = queue.pop_head ();
			var action = child_func (folder_data);
			if (action == ForEachAction.SKIP) {
				continue;
			}
			if (action == ForEachAction.STOP) {
				break;
			}
			var enumerator = yield folder_data.file.enumerate_children_async (
				required_attributes,
				(ForEachFlags.FOLLOW_LINKS in flags) ? FileQueryInfoFlags.NONE : FileQueryInfoFlags.NOFOLLOW_SYMLINKS,
				Priority.DEFAULT,
				cancellable
			);
			while ((info = enumerator.next_file (cancellable)) != null) {
				var child = enumerator.get_child (info);
				var child_data = new Gth.FileData (child, info);
				var child_action = child_func (child_data);
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
		var attributes = (requested_attributes == "*") ? "*" : Util.concat_attributes (REQUIRED_ATTRIBUTES, requested_attributes);
		var info = yield file.query_info_async (attributes, FileQueryInfoFlags.NONE, Priority.DEFAULT, cancellable);
		var file_data = new Gth.FileData (file, info);
		var metadata_attributes_v = Util.extract_metadata_attributes (attributes);
		read_metadata_attributes (file_data, metadata_attributes_v, cancellable);
		return file_data;
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

	public override void monitor_directory (File file, bool activate) {
		// TODO
	}
}
