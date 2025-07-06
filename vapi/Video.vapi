using GLib;

[CCode (cheader_filename = "lib/gstreamer-utils.h")]
namespace Video {
	[CCode (cname = "read_video_metadata")]
	public static bool read_metadata (File file, FileInfo info, Cancellable cancellable) throws Error;
}
