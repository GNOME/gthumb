public class Gth.UnknownViewer : Object, Gth.FileViewer {
	public Gth.ShortcutContext shortcut_context { get { return ShortcutContext.NONE; } }

	public void activate (Gth.Window _window) {
		window = _window;
		var page = new Adw.StatusPage ();
		page.description = _("Cannot view this kind of files");
		page.icon_name = "gth-thumbnail-error-symbolic";
		window.viewer.set_viewer_widget (page);
		window.viewer.set_context_menu (null);
		window.viewer.viewer_container.add_css_class ("image-view");
	}

	public void deactivate () {
		window.viewer.viewer_container.remove_css_class ("image-view");
	}

	public async bool load (FileData file, Job job) throws Error {
		return false;
	}

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

	public void focus () {
	}

	weak Gth.Window window;
}
