public class Gth.DesktopBackground {
	public async void set_file (Gtk.Window window, Gth.FileData file_data, Cancellable cancellable) throws Error {
		var portal = new Xdp.Portal ();
		var parent = Xdp.parent_new_gtk (window);
		yield portal.set_wallpaper (parent,
			file_data.file.get_uri (),
			Xdp.WallpaperFlags.BACKGROUND | Xdp.WallpaperFlags.LOCKSCREEN,
			cancellable
		);
	}
}
