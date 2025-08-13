using GLib;

[CCode (cheader_filename = "lib/exiv2-utils.h", cprefix = "exiv2_")]
namespace Exiv2 {
	// public static bool read_metadata_from_file (
	// 	File file,
	// 	FileInfo info,
	// 	bool update_general_attributes,
	// 	Cancellable cancellable) throws Error;

	[CCode (array_length_type = "size_t", array_length_pos = 1.1)]
	public static bool read_metadata_from_buffer (
		uint8[] buffer,
		FileInfo info,
		bool update_general_attributes) throws Error;

	// public static bool read_sidecar (
	// 	File file,
	// 	FileInfo info,
	// 	bool update_general_attributes);

	// public static void update_general_attributes (FileInfo info);

	// public static bool supports_writes (string mime_type);

	// public static bool write_metadata  (GthImageSaveData *data);

	// public static bool write_metadata_to_buffer (
	// 	out uint8[] buffer,
	// 	FileInfo info,
	// 	GthImage image_data) throws Error;

	// public static bool clear_metadata (ref uint8[] buffer) throws Error;

	// public static Gth.Image generate_thumbnail (
	// 	string uri,
	// 	string mime_type,
	// 	int size);
}
