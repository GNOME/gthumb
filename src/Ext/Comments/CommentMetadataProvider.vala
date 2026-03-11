public class Gth.CommentMetadataProvider : Gth.MetadataProvider {
	const string[] Supported_Attributes = {
		"comment::*",
		"Metadata::DateTime",
		"Metadata::Title",
		"Metadata::Description",
		"Metadata::Place",
		"Metadata::Tags",
		"Metadata::Rating",
	};

	public override bool can_read (File? file, FileInfo info, string[]? attribute_v = null) {
		if (info.get_file_type () != FileType.REGULAR) {
			return false;
		}
		return Util.attributes_match_any_pattern_v (Supported_Attributes, attribute_v);
	}

	public override void read (File? file, Bytes? buffer, FileInfo file_info, Cancellable cancellable) {
		if (file == null) {
			return;
		}
		try {
			var comment_file = Comment.get_comment_file (file);
			var stream = comment_file.read (cancellable);
			// Ignore if the embedded metadata is newer.
			if (file_info.has_attribute ("Embedded::UpdatedGeneralAttributes")
				&& file_info.get_attribute_boolean ("Embedded::UpdatedGeneralAttributes"))
			{
				var comment_info = stream.query_info (FileAttribute.TIME_CHANGED + "," + FileAttribute.TIME_CHANGED_USEC, cancellable);
				var comment_changed_time = Files.get_changed_date_time (comment_info);
				var file_changed_time = Files.get_changed_date_time (file_info);
				if ((file_changed_time != null) && (comment_changed_time != null)) {
					var diff = file_changed_time.difference (comment_changed_time);
					if (diff > 1000000) { // 1 second
						return;
					}
				}
			}
			// Update the metadata from the comment file.
			var bytes = Files.read_all (stream, cancellable, true);
			var comment = new Comment ();
			comment.load_bytes (bytes);
			comment.update_info (file_info);
		}
		catch (Error error) {
			//stdout.printf ("ERROR: %s\n", error.message);
		}
	}
}
