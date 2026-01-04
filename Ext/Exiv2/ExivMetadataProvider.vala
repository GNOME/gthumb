public class Gth.ExivMetadataProvider : Gth.MetadataProvider {
	GLib.Settings settings;

	construct {
		settings = new GLib.Settings (GTHUMB_SCHEMA);
	}

	const string[] Supported_Attributes = {
		"Exif::*",
		"Xmp::*",
		"Iptc::*",
		"Photo::*",
		"Metadata::DateTime",
		"Metadata::Title",
		"Metadata::Description",
		"Metadata::Place",
		"Metadata::Tags"
	};

	public override bool can_read (File? file, FileInfo info, string[]? attribute_v = null) {
		unowned var content_type = Util.get_content_type (file, info);
		if (!ContentType.is_a (content_type, "image/*")
			|| ContentType.is_mime_type (content_type, "image/svg+xml"))
		{
			return false;
		}
		return Util.attributes_match_any_pattern_v (Supported_Attributes, attribute_v);
	}

	public override void read (File? file, Bytes? buffer, FileInfo info, Cancellable cancellable) {
		try {
			Bytes bytes = null;
			if (buffer != null) {
				bytes = buffer;
			}
			else if (file != null) {
				var stream = file.read (cancellable);
				bytes = Files.read_all (stream, cancellable);
			}
			Exiv2.read_metadata_from_buffer (bytes, info);
		}
		catch (Error error) {
			stdout.printf ("ERROR ExivMetadataProvider.read: %s\n", error.message);
		}
	}
}
