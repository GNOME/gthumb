public class Gth.VideoMetadataProvider : Gth.MetadataProvider {
	public override bool can_read (File? file, FileInfo info, string[]? attribute_v = null) {
		unowned var content_type = Util.get_content_type (file, info);
		if (!ContentType.is_a (content_type, "video/*")
			&& !ContentType.is_a (content_type, "audio/*"))
		{
			return false;
		}
		return Util.attributes_match_any_pattern_v (supported_attributes, attribute_v);
	}

	public override bool read (File? file, Bytes? buffer, FileInfo info, Cancellable cancellable) {
		try {
			if (file != null) {
				Video.read_metadata (file, info, cancellable);
				return true;
			}
		}
		catch (Error error) {
		}
		return false;
	}

	construct {
		id = "Video";
		supported_attributes = {
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
		cachable = true;
	}
}
