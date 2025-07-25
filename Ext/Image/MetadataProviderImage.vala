public class Gth.MetadataProviderImage : Gth.MetadataProvider {
	const string[] Supported_Attributes = {
		"general::dimensions",
		"image::width",
		"image::height",
		"frame::width",
		"frame::height",
	};

	public override bool can_read (FileData file_data, string content_type, string[] attribute_v) {
		return Util.attributes_match_any_pattern_v (Supported_Attributes, attribute_v);
	}

	public override void read (FileData file_data, string[] attribute_v, Cancellable cancellable) {
		int width, height;
		if (load_image_info (file_data.file, out width, out height, cancellable)) {
			file_data.info.set_attribute_int32 ("image::width", width);
			file_data.info.set_attribute_int32 ("image::height", height);
			file_data.info.set_attribute_int32 ("frame::width", width);
			file_data.info.set_attribute_int32 ("frame::height", height);
			file_data.info.set_attribute_string ("general::dimensions", "%d × %d".printf (width, height));
		}
	}

	public override bool can_write (FileData file_data, string content_type, string[] attribute_v) {
		return false;
	}

	public override void write (FileData file_data, string[] attribute_v, Cancellable cancellable, Gth.MetadataWriteFlags flags = MetadataWriteFlags.DEFAULT) {
		// void
	}
}
