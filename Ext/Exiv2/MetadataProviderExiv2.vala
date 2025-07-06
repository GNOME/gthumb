public class Gth.MetadataProviderExiv2 : Gth.MetadataProvider {
	GLib.Settings settings;

	construct {
		settings = new GLib.Settings (GTHUMB_SCHEMA);
	}

	const string[] Supported_Attributes = {
		"Exif::*",
		"Xmp::*",
		"Iptc::*",
		"Embedded::Image::*",
		"Embedded::Photo::*",
		"general::datetime",
		"general::title",
		"general::description",
		"general::location",
		"general::tags"
	};

	public override bool can_read (FileData file_data, string content_type, string[] attribute_v) {
		if ((content_type != "*") && !ContentType.is_a (content_type, "image/*")) {
			return false;
		}
		return Util.attributes_match_any_pattern_v (Supported_Attributes, attribute_v);
	}

	public override void read (FileData file_data, string[] attribute_v, Cancellable cancellable) {
		try {
			// The embedded metadata is likely to be outdated if the user chooses to
			// not store metadata in files.
			var update_general_attributes = settings.get_boolean (PREF_GENERAL_STORE_METADATA_IN_FILES);

			var stream = file_data.file.read (cancellable);
			var bytes = Files.read_all (stream, cancellable);
			Exiv2.read_metadata_from_buffer (
				bytes.get_data (),
				file_data.info,
				update_general_attributes);
		}
		catch (Error error) {
			stdout.printf ("ERROR MetadataProviderExiv2.read: %s\n", error.message);
		}
	}

	public override bool can_write (FileData file_data, string content_type, string[] attribute_v) {
		return false;
	}

	public override void write (FileData file_data, string[] attribute_v, Cancellable cancellable, Gth.MetadataWriteFlags flags = MetadataWriteFlags.DEFAULT) {
		// void
	}
}
