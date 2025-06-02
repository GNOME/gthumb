using GLib;

[CCode (cheader_filename = "lib/io/save-png.h")]
public Bytes save_png (Gth.Image image, Cancellable cancellable) throws Error;
