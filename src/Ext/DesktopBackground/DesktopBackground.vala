public class Gth.DesktopBackground {
	public DesktopBackground (Gtk.Window _window) {
		window = _window;
	}

	public async void set_file (Gth.FileData file_data, Cancellable cancellable) throws Error {
		var portal = new Xdp.Portal ();
		var parent = Xdp.parent_new_gtk (window);
		yield portal.set_wallpaper (parent,	file_data.file.get_uri (),
			Xdp.WallpaperFlags.BACKGROUND, cancellable);
	}

	weak Gtk.Window window;
}
