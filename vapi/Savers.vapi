using GLib;

[CCode (cheader_filename = "lib/io/save-png.h")]
public enum Gth.PngOption {
	COMPRESSION_LEVEL,
}

[CCode (cheader_filename = "lib/io/save-png.h")]
public Bytes save_png (
	Gth.Image image,
	[CCode (array_length = false, array_null_terminated = true)] Gth.Option[]? options,
	Cancellable cancellable) throws Error;

[CCode (cheader_filename = "lib/io/save-jpeg.h")]
public enum Gth.JpegOption {
	QUALITY,
	SMOOTHING,
	OPTIMIZE,
	PROGRESSIVE,
}

[CCode (cheader_filename = "lib/io/save-jpeg.h")]
public Bytes save_jpeg (
	Gth.Image image,
	[CCode (array_length = false, array_null_terminated = true)] Gth.Option[]? options,
	Cancellable cancellable) throws Error;
