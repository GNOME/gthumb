public class Gth.ImageMetadataProvider : Gth.MetadataProvider {
	public override bool can_read (File? file, FileInfo info, string[]? attribute_v = null) {
		return Util.attributes_match_any_pattern_v (supported_attributes, attribute_v);
	}

	public override bool read (File? file, Bytes? buffer, FileInfo info, Cancellable cancellable) throws Error {
		int width = 0, height = 0;
		if (buffer != null) {
			if (!load_image_info_from_bytes (buffer, out width, out height, cancellable)) {
				if (cancellable.is_cancelled ()) {
					throw new IOError.CANCELLED ("Cancelled");
				}
				return false;
			}
		}
		else if (file != null) {
			if (!load_image_info (file, out width, out height, cancellable)) {
				if (cancellable.is_cancelled ()) {
					throw new IOError.CANCELLED ("Cancelled");
				}
				return false;
			}
		}
		Lib.set_frame_size (info, width, height);
		return true;
	}

	construct {
		id = "Image";
		supported_attributes = {
			"Frame::Pixels",
			"Frame::Width",
			"Frame::Height",
		};
		cachable = true;
	}
}
