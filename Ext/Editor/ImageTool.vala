public class Gth.ImageTool : Object {
	public weak Window window;
	public weak ImageViewer viewer;
	public string title;
	public string icon_name;

	public void set_window (Window _window) {
		window = _window;
		viewer = window.viewer.current_viewer as ImageViewer;
	}

	public virtual void activate () {}

	public virtual void deactivate () {}

	public virtual ImageOperation? get_operation () {
		return null;
	}

	public async bool apply_changes () {
		var operation = get_operation ();
		if (operation == null) {
			return false;
		}
		return yield viewer.edit_image (title, operation, icon_name);
	}
}
