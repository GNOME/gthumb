public class Gth.MetadataProviderFile : Gth.MetadataProvider {
	const string[] Supported_Attributes = {
		"Private::File::Size",
		"Private::File::DisplaySize",
		"Private::File::DisplayMtime",
		"Private::File::ContentType",
		"Private::File::Location",
	};

	public override bool can_read (FileData file_data, string content_type, string[] attribute_v) {
		return Util.attributes_match_any_pattern_v (Supported_Attributes, attribute_v);
	}

	public override void read (FileData file_data, string[] attribute_v, Cancellable cancellable) {
		var size = "%'lu".printf ((uint64) file_data.info.get_size ());
		file_data.info.set_attribute_string ("Private::File::Size", size);

		var formatted_size = GLib.format_size ((uint64) file_data.info.get_size ());
		file_data.info.set_attribute_string ("Private::File::DisplaySize", formatted_size);

		var time = file_data.get_modification_time ();
		file_data.info.set_attribute_string ("Private::File::DisplayMtime", time.to_display_string ());

		var parent = file_data.file.get_parent ();
		if (parent != null) {
			file_data.info.set_attribute_string ("Private::File::Location", parent.has_uri_scheme ("file") ? parent.get_path () : parent.get_uri ());
		}

		file_data.info.set_attribute_string ("Private::File::ContentType", Util.format_content_type (file_data.get_content_type ()));
	}

	public override bool can_write (FileData file_data, string content_type, string[] attribute_v) {
		return false;
	}

	public override void write (FileData file_data, string[] attribute_v, Cancellable cancellable, Gth.MetadataWriteFlags flags = MetadataWriteFlags.DEFAULT) {
		// void
	}
}
