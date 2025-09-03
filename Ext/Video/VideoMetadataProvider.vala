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

	public override bool can_read (FileData file_data, string[] attribute_v) {
		unowned var content_type = file_data.get_content_type ();
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
}
