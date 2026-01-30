using GLib;

class Zip {
	[CCode (
		cheader_filename = "lib/zlib-utils.h",
		cname = "zlib_decompress",
		array_length_pos = 1.1,
		array_length_pos = 2.1)]
	public static bool decompress (
		[CCode (array_length_type = "size_t")] uint8[] zipped_buffer,
		[CCode (array_length_type = "size_t")] out uint8[] buffer);
}
