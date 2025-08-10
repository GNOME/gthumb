public class Gth.UnknownViewer : Object, Gth.FileViewer {
	public void activate (Gth.Window _window) {
		_window.viewer.set_context_menu (null);
	}

	public async void load (FileData file) throws Error {
		throw new IOError.FAILED (_("Cannot load this kind of file"));
	}

	public void deactivate () {}
	public void save_preferences () {}
	public void release_resources () {}

	public bool on_scroll (double x, double y, double dx, double dy) {
		return false;
	}

	public bool get_pixel_size (out uint width, out uint height) {
		width = 0;
		height = 0;
		return false;
	}
}
