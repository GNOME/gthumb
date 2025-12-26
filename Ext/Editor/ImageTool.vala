public class Gth.ImageTool : Object {
	public string title;
	public string icon_name;
	public weak Window window;
	public weak ImageViewer viewer;
	public Image original;
	public Image resized;
	public Image preview;
	public unowned ImageView image_view;
	public bool resize_preview;
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
		if (!resize_preview) {
			resized = original;
		}
		after_activate ();
		if (image_view != null) {
			window.insert_action_group ("image", image_view.action_group);
		}
	}

	public void deactivate () {
		window.insert_action_group ("image", null);
		before_deactivate ();
		if (preview_job != null) {
			preview_job.cancel ();
		}
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
	}

	public void show_preview (bool show) {
		with_preview = show;
		image_view.image = with_preview ? preview : resized;
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

	public void update_preview_on_resize () {
		if (resized == null) {
			queue_update_preview ();
		}
	}

	uint preview_count = 0;

	public void update_preview () {
		if (image_view == null) {
			return;
		}
		if (preview_job != null) {
			preview_job.cancel ();
		}
		preview_count++;
		var job = window.new_job ("Update Preview");
		preview_job = job;
		preview_filter_async.begin (job.cancellable, (_obj, res) => {
			try {
				// stdout.printf ("> update_preview [3][%u]\n", preview_count);
				preview = preview_filter_async.end (res);
				if (with_preview) {
					// stdout.printf ("> update_preview [4][%u] image: %p\n", preview_count, preview);
					image_view.image = preview;
				}
			}
			catch (Error error) {
				// stdout.printf ("> update_preview [6][%u] error: %s\n", preview_count, error.message);
				window.show_error (error);
			}
			finally {
				job.done ();
				if (job == preview_job) {
					preview_job = null;
				}
			}
		});
	}

	uint get_preview_size () {
		int width = image_view.get_width ();
		int height = image_view.get_height ();
		return (uint) (((original.width > original.height) ? width : height));
	}

	async Image? preview_filter_async (Cancellable cancellable) throws Error {
		// stdout.printf ("> preview_filter_async [1][%u]\n", preview_count);
		if (resized == null) {
			var new_size = get_preview_size ();
			if (new_size == 0) {
				throw new IOError.CANCELLED ("Cancelled");
			}
			// stdout.printf ("> new_size: %u\n", new_size);
			resized = yield original.resize_async (new_size, ResizeFlags.DEFAULT, ScaleFilter.BOX, cancellable);
			image_view.image = resized;
		}
		var operation = get_operation ();
		if (operation == null) {
			return resized;
		}
		update_options_from_operation (operation);
		return yield app.image_editor.exec_operation (resized, operation, cancellable);
	}

	construct {
		resized = null;
		image_view = null;
		preview_job = null;
		resize_preview = true;
		with_preview = true;
		shortcut_context = Gth.ShortcutContext.IMAGE_EDITOR;
	}

	Job preview_job;
	const uint UPDATE_DELAY = 50;
}
