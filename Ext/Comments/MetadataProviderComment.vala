public class Gth.MetadataProviderComment : Gth.MetadataProvider {
	const string[] Supported_Attributes = {
		"comment::*",
		"general::datetime",
		"general::title",
		"general::description",
		"general::location",
		"general::tags",
		"general::rating",
	};

	public override bool can_read (FileData file_data, string content_type, string[] attribute_v) {
		if ((content_type != "*") && !ContentType.is_a (content_type, "image/*")) {
			return false;
		}
		return Util.attributes_match_any_pattern_v (Supported_Attributes, attribute_v);
	}

	public override void read (FileData file_data, string[] attribute_v, Cancellable cancellable) {
		try {
			var file = Comment.get_comment_file (file_data.file);
			var bytes = Files.load_file (file, cancellable);
			var comment = new Comment ();
			comment.load_bytes (bytes);

			file_data.info.set_attribute_boolean ("comment::no-comment-file", false);
			if (!Strings.empty (comment.note)) {
				file_data.info.set_attribute_string ("comment::note", comment.note);
				file_data.info.set_attribute_object ("general::description", new Metadata.for_string (comment.note));
			}

			if (!Strings.empty (comment.caption)) {
				file_data.info.set_attribute_string ("comment::caption", comment.caption);
				file_data.info.set_attribute_object ("general::title", new Metadata.for_string (comment.caption));
			}

			if (!Strings.empty (comment.place)) {
				file_data.info.set_attribute_string ("comment::place", comment.place);
				file_data.info.set_attribute_object ("general::location", new Metadata.for_string (comment.place));
			}

			if (comment.rating > 0) {
				file_data.info.set_attribute_int32 ("comment::rating", comment.rating);
				file_data.info.set_attribute_object ("general::rating", new Metadata.for_string ("%d".printf (comment.rating)));
			}
			else {
				file_data.info.remove_attribute ("comment::rating");
				file_data.info.remove_attribute ("general::rating");
			}

			if (comment.categories.length > 0) {
				var list = new StringList.from_array (comment.categories);
				var metadata = new Metadata.for_string_list (list);
				file_data.info.set_attribute_object ("comment::categories", metadata);
				file_data.info.set_attribute_object ("general::tags", metadata);
			}
			else {
				file_data.info.remove_attribute ("comment::categories");
				file_data.info.remove_attribute ("general::tags");
			}

			if (comment.time.date_is_valid ()) {
				var raw = comment.time.to_exif_date ();
				var formatted = comment.time.to_display_string ();
				var metadata = new Metadata.for_string (raw, formatted);
				file_data.info.set_attribute_object ("comment::time", metadata);
				file_data.info.set_attribute_object ("general::datetime", metadata);
			}
			else {
				file_data.info.remove_attribute ("comment::time");
				file_data.info.remove_attribute ("general::datetime");
			}

		}
		catch (Error error) {
			stdout.printf ("ERRROR: %s\n", error.message);
			file_data.info.set_attribute_boolean ("comment::no-comment-file", true);
		}
	}

	public override bool can_write (FileData file_data, string content_type, string[] attribute_v) {
		return false;
	}

	public override void write (FileData file_data, string[] attribute_v, Cancellable cancellable, Gth.MetadataWriteFlags flags = MetadataWriteFlags.DEFAULT) {
		// void
	}
}
