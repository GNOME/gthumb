using GLib;

[CCode (cheader_filename = "lib/io/image-info.h")]
public bool load_image_info (File file, out int width, out int height, Cancellable cancellable);

[CCode (cheader_filename = "lib/io/image-info.h")]
public bool load_image_info_from_bytes (Bytes bytes, out int width, out int height, Cancellable cancellable);

[CCode (cheader_filename = "lib/io/image-info.h")]
public bool load_image_info_from_stream (InputStream stream, out int width, out int height, Cancellable cancellable);

[CCode (cheader_filename = "lib/util.h", array_length_type = "size_t", array_length_pos = 1.1)]
public unowned string guess_mime_type (uint8[] buffer);

[CCode (cheader_filename = "lib/io/load-png.h")]
public Gth.Image load_png (Bytes bytes, uint requested_size, Cancellable cancellable) throws Error;

[CCode (cheader_filename = "lib/io/load-png.h")]
public HashTable<string, string> load_png_attributes (Bytes bytes) throws Error;

[CCode (cheader_filename = "lib/io/load-jpeg.h")]
public Gth.Image load_jpeg (Bytes bytes, uint requested_size, Cancellable cancellable) throws Error;

[CCode (cheader_filename = "lib/io/load-webp.h")]
public Gth.Image load_webp (Bytes bytes, uint requested_size, Cancellable cancellable) throws Error;

[CCode (cheader_filename = "lib/io/load-svg.h")]
public Gth.Image load_svg (Bytes bytes, uint requested_size, Cancellable cancellable) throws Error;

[CCode (cheader_filename = "lib/io/load-jxl.h")]
public Gth.Image load_jxl (Bytes bytes, uint requested_size, Cancellable cancellable) throws Error;

[CCode (cheader_filename = "lib/io/load-heif.h")]
public Gth.Image load_heif (Bytes bytes, uint requested_size, Cancellable cancellable) throws Error;

[CCode (cheader_filename = "lib/io/load-tiff.h")]
public Gth.Image load_tiff (Bytes bytes, uint requested_size, Cancellable cancellable) throws Error;

[CCode (cheader_filename = "lib/io/load-gif.h")]
public Gth.Image load_gif (Bytes bytes, uint requested_size, Cancellable cancellable) throws Error;
