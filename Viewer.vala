[GtkTemplate (ui = "/app/gthumb/gthumb/ui/viewer.ui")]
public class Gth.Viewer : Gtk.Box {
	public weak Window window {
		get { return _window; }
		set {
			_window = value;
			init ();
		}
	}

	weak Window _window;

	public void view_file (FileData file) {
		// TODO
		window.set_page (Window.Page.VIEWER);
	}

	void init () {
		// TODO
	}
}
