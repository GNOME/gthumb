public class Gth.ImageMetadataProvider : Gth.MetadataProvider {
	const string[] Supported_Attributes = {
		"Frame::Pixels",
		"Frame::Width",
		"Frame::Height",
	};

	public override bool can_read (FileData file_data, string[] attribute_v) {
		return Util.attributes_match_any_pattern_v (Supported_Attributes, attribute_v);
	}

	public override void read (FileData file_data, string[] attribute_v, Cancellable cancellable) {
		int width, height;
		if (load_image_info (file_data.file, out width, out height, cancellable)) {
			file_data.info.set_attribute_int32 ("Frame::Width", width);
			file_data.info.set_attribute_int32 ("Frame::Height", height);
			file_data.info.set_attribute_string ("Frame::Pixels", "%d × %d".printf (width, height));
		}
	}
}
