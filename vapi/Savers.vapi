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

[CCode (cheader_filename = "lib/io/save-webp.h")]
public enum Gth.WebpOption {
	QUALITY,
	METHOD,
	LOSSLESS,
}

[CCode (cheader_filename = "lib/io/save-webp.h")]
public Bytes save_webp (
	Gth.Image image,
	[CCode (array_length = false, array_null_terminated = true)] Gth.Option[]? options,
	Cancellable cancellable) throws Error;

[CCode (cheader_filename = "lib/io/save-avif.h")]
public enum Gth.AvifOption {
	QUALITY,
	LOSSLESS,
}

[CCode (cheader_filename = "lib/io/save-avif.h")]
public Bytes save_avif (
	Gth.Image image,
	[CCode (array_length = false, array_null_terminated = true)] Gth.Option[]? options,
	Cancellable cancellable) throws Error;

[CCode (cheader_filename = "lib/io/save-tiff.h")]
public enum Gth.TiffOption {
	DEFAULT_EXT,
	COMPRESSION,
	HRESOLUTION,
	VRESOLUTION,
}

[CCode (cheader_filename = "lib/io/save-tiff.h")]
public enum Gth.TiffCompression {
	NONE,
	LOSSLESS,
	JPEG,
}

[CCode (cheader_filename = "lib/io/save-tiff.h")]
public Bytes save_tiff (
	Gth.Image image,
	[CCode (array_length = false, array_null_terminated = true)] Gth.Option[]? options,
	Cancellable cancellable) throws Error;
