using GLib;

[CCode (cheader_filename = "lib/io/load-png.h")]
public Gth.Image load_png (Bytes bytes, uint requested_size, Cancellable cancellable) throws Error;
