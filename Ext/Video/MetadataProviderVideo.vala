public class Gth.MetadataProviderVideo : Gth.MetadataProvider {
	const string[] Supported_Attributes = {
		"general::title",
		"general::format",
		"general::dimensions",
		"general::duration",
		"frame::width",
		"frame::height",
		"audio-video::*"
	};

	public override bool can_read (FileData file_data, string content_type, string[] attribute_v) {
		if ((content_type != "*")
			&& !ContentType.is_a (content_type, "video/*")
			&& !ContentType.is_a (content_type, "audio/*"))
		{
			return false;
		}
		return Util.attributes_match_any_pattern_v (Supported_Attributes, attribute_v);
	}

	public override void read (FileData file_data, string[] attribute_v, Cancellable cancellable) {
		try {
			Video.read_metadata (file_data.file, file_data.info, cancellable);
		}
		catch (Error error) {
		}
	}

	public override bool can_write (FileData file_data, string content_type, string[] attribute_v) {
		return false;
	}

	public override void write (FileData file_data, string[] attribute_v, Cancellable cancellable, Gth.MetadataWriteFlags flags = MetadataWriteFlags.DEFAULT) {
		// void
	}
}
