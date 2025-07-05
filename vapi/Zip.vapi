using GLib;

class Zip {
	[CCode (
		cheader_filename = "lib/zlib-utils.h",
		cname = "zlib_decompress",
		array_length_type = "size_t",
		array_length_pos = 1.1)]
	public static bool decompress (uint8[] zipped_buffer, out uint8[] buffer);
}
