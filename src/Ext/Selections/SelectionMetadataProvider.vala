public class Gth.SelectionMetadataProvider : Gth.MetadataProvider {
	public override bool can_read (File? file, FileInfo info, string[]? attribute_v = null) {
		return (info.get_file_type () == FileType.REGULAR)
			&& Util.attributes_match_any_pattern_v (supported_attributes, attribute_v);
	}

	public override bool read (File? file, Bytes? buffer, FileInfo file_info, Cancellable cancellable) throws Error {
		if (file == null) {
			return false;
		}
		app.selections.update_private_attribute (file, file_info);
		return true;
	}

	construct {
		id = "Selection";
		supported_attributes = {
			PrivateAttribute.SELECTIONS,
		};
		cachable = false;
	}
}
