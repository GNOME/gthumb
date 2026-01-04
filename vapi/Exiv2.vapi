using GLib;

[CCode (cheader_filename = "lib/exiv2-utils.h", cprefix = "exiv2_")]
namespace Exiv2 {
	[CCode (array_length_type = "size_t", array_length_pos = 1.1)]
	public static bool read_metadata_from_buffer (Bytes buffer, FileInfo info, bool update_general_attributes = true) throws Error;
	public static bool can_write_metadata (string mime_type);
	public static Bytes write_metadata_to_buffer (Bytes buffer, FileInfo info, Gth.Image? image_data = null, bool update_from_general_attributes = true) throws Error;
	public static Bytes clear_metadata (Bytes buffer) throws Error;
	public static void update_dimensions (FileInfo info, Gth.Transform transform);

	[CCode (cname = "_ORIGINAL_DATE_TAG_NAMES", array_length = false, array_null_terminated = true)]
	public string[] ORIGINAL_DATE_TAG_NAMES;
}
