public class Gth.ImageViewer : Object, Gth.FileViewer {
	public void activate (Gth.Window _window) {
		assert (window == null);
		window = _window;
		image_view = new Gth.ImageView ();
		image_view.zoom_type = settings.get_enum (PREF_IMAGE_ZOOM_TYPE);
		window.viewer.set_viewer_widget (image_view);
		window.viewer.viewer_container.add_css_class ("image-view");
		window.viewer.set_statusbar_maximized (false);
		init_actions ();
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/image-viewer.ui");
		window.viewer.set_left_toolbar (builder.get_object ("left_toolbar") as Gtk.Widget);

		var zoom_button = builder.get_object ("zoom_button") as Gtk.Widget;
		zoom_button.remove_css_class ("popup");

		unowned var adj = builder.get_object ("zoom_adjustment") as Gtk.Adjustment;
		zoom_adj_changed_id = adj.value_changed.connect ((local_adj) => {
			var x = local_adj.get_value ();
			x = (adj_to_zoom (x) - adj_to_zoom (ZOOM_ADJ_MIN)) / (adj_to_zoom (ZOOM_ADJ_MAX) - adj_to_zoom (ZOOM_ADJ_MIN));
			var zoom = ZOOM_MIN + (x * (ZOOM_MAX - ZOOM_MIN));
			image_view.zoom = (float) zoom.clamp (ZOOM_MIN, ZOOM_MAX);
		});

		unowned var zoom_scale = builder.get_object ("zoom_scale") as Gtk.Widget;
		unowned var zoom_popover = builder.get_object ("zoom_popover") as Gtk.PopoverMenu;
		zoom_popover.add_child (zoom_scale, "scale");

		image_view.resized.connect (() => update_zoom_info ());

		var click_events = new Gtk.GestureClick ();
		click_events.button = Gdk.BUTTON_PRIMARY;
		click_events.pressed.connect ((n_press, x, y) => {
			//stdout.printf ("> PRESSED %f,%f\n", x, y);
			clicking = true;
			dragging = false;
			drag_start = { x, y };
		});
		click_events.released.connect ((n_press, x, y) => {
			//stdout.printf ("> RELEASED\n");
			clicking = false;
			dragging = false;
		});
		image_view.add_controller (click_events);

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((x, y) => {
			if (!dragging && clicking) {
				var dx = (drag_start.x - x).abs ();
				var dy = (drag_start.y - y).abs ();
				//stdout.printf ("delta: %f,%f\n", dx, dy);
				if ((dx >= DRAG_THRESHOLD) || (dy >= DRAG_THRESHOLD)) {
					dragging = true;
				}
			}
			if (dragging) {
				var dx = (float) (drag_start.x - x);
				var dy = (float) (drag_start.y - y);
				//stdout.printf ("scroll_by: %f,%f\n", dx, dy);
				image_view.scroll_by (dx, dy);
				drag_start = { x, y };
			}
		});
		image_view.add_controller (motion_events);
	}

	public async void load (FileData file_data) throws Error {
		if (load_job != null) {
			load_job.cancel ();
		}
		var local_job = app.new_job ("Load image %s".printf (file_data.file.get_uri ()));
		load_job = local_job;
		try {
			var image = yield app.image_loader.load_file (file_data.file, local_job.cancellable);
			var monitor_profile = yield window.get_monitor_profile (local_job.cancellable);
			if (monitor_profile != null) {
				yield image.apply_icc_profile_async (app.color_manager, monitor_profile, local_job.cancellable);
			}
			image_view.image = image;
			file_data.set_content_type (image_view.image.get_attribute ("content-type"));
		}
		catch (Error error) {
			stdout.printf ("ImageViewer.load: ERROR: %s\n", error.message);
			local_job.error = error;
		}
		local_job.done ();
		if (load_job == local_job) {
			load_job = null;
		}
		if (local_job.error != null) {
			throw local_job.error;
		}
	}

	public void deactivate () {
		window.viewer.viewer_container.remove_css_class ("image-view");
		window.insert_action_group ("image", null);
		settings.set_enum (PREF_IMAGE_ZOOM_TYPE, image_view.zoom_type);
	}

	public void show () {
	}

	public void hide () {
	}

	void init_actions () {
		action_group = new SimpleActionGroup ();
		window.insert_action_group ("image", action_group);

		var action = new SimpleAction.stateful ("zoom-100", null, new Variant.boolean (false));
		action.activate.connect ((action, param) => {
			image_view.zoom_type = ZoomType.NATURAL_SIZE;
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("zoom-best-fit", null, new Variant.boolean (false));
		action.activate.connect ((_action, param) => {
			image_view.zoom_type = ZoomType.BEST_FIT;
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("set-zoom", VariantType.STRING, new Variant.string (""));
		action.activate.connect ((action, param) => {
			unowned var value = param.get_string ();
			action.set_state (new Variant.string (value));
			switch (value) {
			case "best-fit":
				image_view.zoom_type = ZoomType.BEST_FIT;
				break;
			case "max-size":
				image_view.zoom_type = ZoomType.MAXIMIZE;
				break;
			case "max-width":
				image_view.zoom_type = ZoomType.MAXIMIZE_WIDTH;
				break;
			case "max-height":
				image_view.zoom_type = ZoomType.MAXIMIZE_HEIGHT;
				break;
			case "fill-space":
				image_view.zoom_type = ZoomType.FILL_SPACE;
				break;
			case "50":
				image_view.zoom = 0.5f;
				break;
			case "100":
				image_view.zoom = 1f;
				break;
			case "200":
				image_view.zoom = 2f;
				break;
			case "300":
				image_view.zoom = 3f;
				break;
			}
		});
		action_group.add_action (action);
	}

	void update_zoom_info () {
		if (image_view.image == null) {
			window.viewer.status.set_pixel_info (0, 0);
			window.viewer.status.set_zoom_info (0);
			return;
		}

		// Update the statusbar.
		uint width, height;
		image_view.image.get_natural_size (out width, out height);
		window.viewer.status.set_pixel_info (width, height);
		window.viewer.status.set_zoom_info (image_view.zoom);

		// Update the 'set-zoom' action state.
		var action = action_group.lookup_action ("set-zoom") as SimpleAction;
		if (action != null) {
			var state = "";
			switch (image_view.zoom_type) {
			case ZoomType.BEST_FIT:
				state = "best-fit";
				break;
			case ZoomType.MAXIMIZE:
				state = "max-size";
				break;
			case ZoomType.MAXIMIZE_WIDTH:
				state = "max-width";
				break;
			case ZoomType.MAXIMIZE_HEIGHT:
				state = "max-height";
				break;
			case ZoomType.FILL_SPACE:
				state = "fill-space";
				break;
			default:
				if (image_view.zoom == 0.5f) {
					state = "50";
				}
				else if (image_view.zoom == 1f) {
					state = "100";
				}
				else if (image_view.zoom == 2f) {
					state = "200";
				}
				else if (image_view.zoom == 3f) {
					state = "300";
				}
				break;
			}
			action.set_state (new Variant.string (state));
		}

		action = action_group.lookup_action ("zoom-100") as SimpleAction;
		if (action != null) {
			action.set_state (new Variant.boolean (image_view.zoom_type == ZoomType.NATURAL_SIZE));
		}

		action = action_group.lookup_action ("zoom-best-fit") as SimpleAction;
		if (action != null) {
			action.set_state (new Variant.boolean (image_view.zoom_type == ZoomType.BEST_FIT));
		}

		// Update the zoom adjustment.
		unowned var adj = builder.get_object ("zoom_adjustment") as Gtk.Adjustment;
		SignalHandler.block (adj, zoom_adj_changed_id);
		var x = ((double) image_view.zoom - ZOOM_MIN) / (ZOOM_MAX - ZOOM_MIN);
		x = x * (adj_to_zoom (ZOOM_ADJ_MAX) - adj_to_zoom (ZOOM_ADJ_MIN)) + adj_to_zoom (ZOOM_ADJ_MIN);
		x = zoom_to_adj (x);
		x = x.clamp (ZOOM_ADJ_MIN, ZOOM_ADJ_MAX);
		adj.set_value (x);
		SignalHandler.unblock (adj, zoom_adj_changed_id);

		window.viewer.reveal_overlay_controls ();
	}

	inline double adj_to_zoom (double x) {
		return Math.exp (x / 15 - Math.E);
	}

	inline double zoom_to_adj (double z) {
		return (Math.log (z) + Math.E) * 15;
	}

	construct {
		settings = new GLib.Settings (GTHUMB_IMAGES_SCHEMA);
		window = null;
		builder = null;
		load_job = null;
		dragging = false;
		clicking = false;
	}

	weak Gth.Window window;
	GLib.Settings settings;
	Gtk.Builder builder;
	Gth.Job load_job;
	Gth.ImageView image_view;
	ulong zoom_adj_changed_id;
	SimpleActionGroup action_group;
	bool dragging;
	bool clicking;
	ClickPoint drag_start;

	const float ZOOM_MIN = 0.05f;
	const float ZOOM_MAX = 10f;
	const float ZOOM_ADJ_MIN = 0f;
	const float ZOOM_ADJ_MAX = 100f;
	const double DRAG_THRESHOLD = 1;
}
