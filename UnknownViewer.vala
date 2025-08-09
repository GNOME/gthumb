public class Gth.UnknownViewer : Object, Gth.FileViewer {
	public async void load (FileData file) throws Error {
		throw new IOError.FAILED (_("Cannot load this kind of file"));
	}

	public void activate (Gth.Window _window) {}
	public void deactivate () {}
	public void show () {}
	public void hide () {}

	public bool on_scroll (double x, double y, double dx, double dy) {
		return false;
	}

	public bool get_pixel_size (out uint width, out uint height) {
		width = 0;
		height = 0;
		return false;
	}
}
