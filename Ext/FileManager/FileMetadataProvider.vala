
public class Gth.FileMetadataProvider : Gth.MetadataProvider {
	const string[] Supported_Attributes = {
		"Private::File::Size",
		"Private::File::DisplaySize",
		"Private::File::DisplayMtime",
		"Private::File::ContentType",
		"Private::File::Location",
	};

	public override bool can_read (File? file, FileInfo info, string[]? attribute_v = null) {
		return Util.attributes_match_any_pattern_v (Supported_Attributes, attribute_v);
	}

	public override void read (File? file, Bytes? buffer, FileInfo info, Cancellable cancellable) {
		var size = "%'lu".printf ((uint64) info.get_size ());
		info.set_attribute_string ("Private::File::Size", size);

		var formatted_size = GLib.format_size ((uint64) info.get_size ());
		info.set_attribute_string ("Private::File::DisplaySize", formatted_size);

		var time = info.get_modification_date_time ();
		if (time != null) {
			var mtime = new Gth.DateTime ();
			mtime.set_from_gdatetime (time);
			info.set_attribute_string ("Private::File::DisplayMtime", mtime.to_display_string ());
		}

		if (file != null) {
			var parent = file.get_parent ();
			if (parent != null) {
				info.set_attribute_string ("Private::File::Location",
					parent.has_uri_scheme ("file") ? parent.get_path () : parent.get_uri ());
			}
		}

		info.set_attribute_string ("Private::File::ContentType",
			Util.format_content_type (Util.get_content_type (file, info)));
	}
}
