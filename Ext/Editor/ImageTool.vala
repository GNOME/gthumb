public class Gth.ImageTool : Object {
	public string title;
	public string icon_name;
	public weak Window window;
	public weak ImageViewer viewer;
	public Image original;
	public unowned ImageView image_view;
	public bool with_preview;
	public Gth.ShortcutContext shortcut_context;

	public virtual void after_activate () {}

	public virtual void before_deactivate () {}

	public virtual ImageOperation? get_operation () {
		return null;
	}

	public virtual void update_options_from_operation (ImageOperation operation) {
		// void
	}

	public virtual void focus () {
		if (image_view != null) {
			image_view.grab_focus ();
		}
	}

	public void activate (Window _window) {
		window = _window;
		viewer = window.viewer.current_viewer as ImageViewer;
		original = viewer.image_view.image;
		after_activate ();
		if (image_view != null) {
			window.insert_action_group ("image", image_view.action_group);
		}
	}

	public void deactivate () {
		window.insert_action_group ("image", null);
		before_deactivate ();
		cancel_update_preview ();
	}

	public async bool apply_changes () {
		var operation = get_operation ();
		if (operation == null) {
			return false;
		}
		return yield viewer.edit_image (title, operation, icon_name);
	}

	public void add_default_controllers (ImageView _image_view) {
		image_view.add_drag_controller ();
		image_view.add_scroll_controller ();

		var key_events = new Gtk.EventControllerKey ();
		key_events.key_pressed.connect (window.on_key_pressed);
		image_view.add_controller (key_events);

		image_view.image = original;
		image_view.set_first_state_from_view (viewer.image_view);
	}

	public void show_preview (bool show) {
		with_preview = show;
		queue_update_preview ();
	}

	uint update_event = 0;

	public void cancel_update_preview () {
		if (update_event != 0) {
			Source.remove (update_event);
			update_event = 0;
		}
	}

	public void queue_update_preview () {
		if (update_event != 0) {
			return;
		}
		update_event = Util.after_timeout (UPDATE_DELAY, () => {
			update_event = 0;
			update_preview ();
		});
	}

	public void update_preview () {
		if (image_view == null) {
			return;
		}
		image_view.filter_operation = with_preview ? get_operation () : null;
	}

	construct {
		image_view = null;
		with_preview = true;
		shortcut_context = Gth.ShortcutContext.IMAGE_EDITOR;
	}

	const uint UPDATE_DELAY = 50;
}
