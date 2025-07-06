using GLib;

[CCode (cheader_filename = "lib/gstreamer-utils.h")]
namespace Video {
	[CCode (cname = "gstreamer_read_metadata_from_file")]
	public static bool read_metadata (File file, FileInfo info, Cancellable cancellable) throws Error;
}
