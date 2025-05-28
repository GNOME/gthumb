using GLib;

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

public enum Gth.ColorSpace {
	UNKNOWN,
	SRGB,
	ADOBERGB,
	UNCALIBRATED,
}

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

public const Gth.Format GTH_PIXEL_FORMAT;
