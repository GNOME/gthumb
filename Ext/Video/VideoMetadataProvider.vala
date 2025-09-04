public class Gth.VideoMetadataProvider : Gth.MetadataProvider {
	const string[] Supported_Attributes = {
		"Metadata::Title",
		"Metadata::Description",
		"Metadata::Format",
		"Metadata::Duration",
		"Frame::Pixels",
		"Frame::Width",
		"Frame::Height",
		"Media::*",
		"Audio::*",
		"Video::*",
	};

	public override bool can_read (File? file, FileInfo info, string[]? attribute_v = null) {
		unowned var content_type = Util.get_content_type (file, info);
		if (!ContentType.is_a (content_type, "video/*")
			&& !ContentType.is_a (content_type, "audio/*"))
		{
			return false;
		}
		return Util.attributes_match_any_pattern_v (Supported_Attributes, attribute_v);
	}

	public override void read (File? file, Bytes? buffer, FileInfo info, Cancellable cancellable) {
		try {
			if (file != null) {
				Video.read_metadata (file, info, cancellable);
			}
		}
		catch (Error error) {
		}
	}
}
