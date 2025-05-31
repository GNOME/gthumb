using GLib;

[CCode (cheader_filename = "lib/types.h")]
public enum Gth.Transform {
	NONE = 1,
	FLIP_H,
	ROTATE_180,
	FLIP_V,
	TRANSPOSE,
	ROTATE_90,
	TRANSVERSE,
	ROTATE_270
}

[CCode (cheader_filename = "lib/types.h")]
public enum Gth.ColorSpace {
	UNKNOWN,
	SRGB,
	ADOBERGB,
	UNCALIBRATED,
}

[CCode (cheader_filename = "lib/types.h")]
public enum Gth.Format {
	RGBA,
	ABGR;

	// Gdk.MemoryFormat to_mem_format () {
	// 	return MEM_FORMAT[this];
	// }

	// const Gdk.MemoryFormat[] MEM_FORMAT = {
	// 	Gdk.MemoryFormat.R8G8B8A8_PREMULTIPLIED,
	// 	Gdk.MemoryFormat.A8B8G8R8_PREMULTIPLIED,
	// };
}

[CCode (cheader_filename = "lib/types.h")]
public const Gth.Format GTH_PIXEL_FORMAT;

[CCode (cheader_filename = "lib/types.h")]
public enum Gth.ScaleFilter {
	POINT,
	BOX,
	TRIANGLE,
	CUBIC,
	LANCZOS2,
	LANCZOS3,
	MITCHELL_NETRAVALI,
	FAST,
	BEST,
	GOOD,
}
