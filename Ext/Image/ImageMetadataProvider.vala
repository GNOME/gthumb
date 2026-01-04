public class Gth.ImageMetadataProvider : Gth.MetadataProvider {
	const string[] Supported_Attributes = {
		"Frame::Pixels",
		"Frame::Width",
		"Frame::Height",
	};

	public override bool can_read (File? file, FileInfo info, string[]? attribute_v = null) {
		return Util.attributes_match_any_pattern_v (Supported_Attributes, attribute_v);
	}

	public override void read (File? file, Bytes? buffer, FileInfo info, Cancellable cancellable) {
		int width = 0, height = 0;
		if (buffer != null) {
			if (!load_image_info_from_bytes (buffer, out width, out height, cancellable)) {
				return;
			}
		}
		else if (file != null) {
			if (!load_image_info (file, out width, out height, cancellable)) {
				return;
			}
		}
		Lib.set_frame_size (info, width, height);
	}
}
