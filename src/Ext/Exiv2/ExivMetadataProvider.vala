public class Gth.ExivMetadataProvider : Gth.MetadataProvider {
	public override bool can_read (File? file, FileInfo info, string[]? attribute_v = null) {
		unowned var content_type = Util.get_content_type (file, info);
		if (!ContentType.is_a (content_type, "image/*")
			|| ContentType.is_mime_type (content_type, "image/svg+xml"))
		{
			return false;
		}
		return Util.attributes_match_any_pattern_v (supported_attributes, attribute_v);
	}

	public override bool read (File? file, Bytes? buffer, FileInfo info, Cancellable cancellable) {
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
			return true;
		}
		catch (Error error) {
			stdout.printf ("ERROR ExivMetadataProvider.read: %s\n", error.message);
			return false;
		}
	}

	construct {
		id = "Exiv";
		supported_attributes = {
			"Exif::*",
			"Xmp::*",
			"Iptc::*",
			"Photo::*",
			"Metadata::DateTime",
			"Metadata::Title",
			"Metadata::Description",
			"Metadata::Place",
			"Metadata::Tags",
			"Metadata::Rating",
		};
		cachable = true;
	}
}
