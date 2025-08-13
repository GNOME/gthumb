public class Gth.ImageViewer : Object, Gth.FileViewer {
	public void activate (Gth.Window _window) {
		assert (window == null);

		window = _window;

		typeof (Gth.ImageView).ensure ();
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/image-viewer.ui");
		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.zoom_type = settings.get_enum (PREF_IMAGE_ZOOM_TYPE);
		image_view.transparency = settings.get_enum (PREF_IMAGE_TRANSPARENCY);
		scroll_action = settings.get_enum (PREF_IMAGE_SCROLL_ACTION);
		window.viewer.set_viewer_widget (builder.get_object ("main_view") as Gtk.Widget);
		window.viewer.set_context_menu (builder.get_object ("context_menu") as Menu);
		window.viewer.viewer_container.add_css_class ("image-view");
		window.viewer.set_left_toolbar (builder.get_object ("left_toolbar") as Gtk.Widget);
		window.viewer.set_mediabar (builder.get_object ("mediabar") as Gtk.Widget, Gtk.Align.CENTER);
		zoom_info = builder.get_object ("zoom_info") as Gtk.Label;

		var zoom_button = builder.get_object ("zoom_button") as Gtk.Widget;
		zoom_button.notify["active"].connect ((obj, spec) => {
			window.viewer.on_actived_popup ((obj as Gtk.MenuButton).active);
		});

		var options_button = builder.get_object ("options_button") as Gtk.Widget;
		options_button.notify["active"].connect ((obj, spec) => {
			window.viewer.on_actived_popup ((obj as Gtk.MenuButton).active);
		});

		zoom_adjustment = builder.get_object ("zoom_adjustment") as Gtk.Adjustment;
		zoom_adj_changed_id = zoom_adjustment.value_changed.connect ((local_adj) => {
			var x = local_adj.get_value ();
			x = (adj_to_zoom (x) - adj_to_zoom (ZOOM_ADJ_MIN)) / (adj_to_zoom (ZOOM_ADJ_MAX) - adj_to_zoom (ZOOM_ADJ_MIN));
			var zoom = ZOOM_MIN + (x * (ZOOM_MAX - ZOOM_MIN));
			image_view.zoom = (float) zoom.clamp (ZOOM_MIN, ZOOM_MAX);
		});

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

		init_actions ();
	}

	public async void view_image (Gth.Image image, Gth.FileData? file_data, Cancellable cancellable) {
		try {
			var icc_profile = image.get_icc_profile ();
			if (icc_profile != null) {
				var monitor_profile = yield window.get_monitor_profile (cancellable);
				if (monitor_profile != null) {
					yield image.apply_icc_profile_async (app.color_manager, monitor_profile, cancellable);
				}
				unowned var description = icc_profile.get_description ();
				if (description != null) {
					file_data.info.set_attribute_string (PrivateAttribute.LOADED_IMAGE_COLOR_PROFILE, description);
				}
			}
		}
		catch (Error error) {
			window.show_error (error);
		}
		image_view.image = image;
	}

	public async void load (FileData file_data) throws Error {
		if (load_job != null) {
			load_job.cancel ();
		}
		var local_job = app.new_job ("Load image %s".printf (file_data.file.get_uri ()));
		load_job = local_job;
		try {
			var image = yield app.image_loader.load_file (file_data.file, local_job.cancellable);
			yield view_image (image, file_data, local_job.cancellable);
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
	}

	public void save_preferences () {
		settings.set_enum (PREF_IMAGE_ZOOM_TYPE, image_view.zoom_type);
		settings.set_enum (PREF_IMAGE_TRANSPARENCY, image_view.transparency);
	}

	public void release_resources () {
		// TODO
	}

	public bool on_scroll (double x, double y, double dx, double dy) {
		switch (scroll_action) {
		case ScrollAction.CHANGE_FILE:
			return window.viewer.on_scroll_change_file (dx, dy);

		case ScrollAction.CHANGE_ZOOM:
			var step = (dy < 0) ? 0.1f : -0.1f;
			image_view.zoom = image_view.zoom + (image_view.zoom * step);
			break;

		default:
			break;
		}
		return false;
	}

	public bool get_pixel_size (out uint width, out uint height) {
		if (image_view.image == null)
			return false;
		image_view.image.get_natural_size (out width, out height);
		return true;
	}

	public override async void save () throws Error {
		var local_job = window.new_job ("Saving File");
		var current_file = window.viewer.current_file;
		try {
			// TODO: load the original image if a lower quality version was loaded.

			// Save LOADED_IMAGE_IS_MODIFIED into LOADED_IMAGE_WAS_MODIFIED
			// to allow the exiv2 metadata writer to not change some fields if the
			// content wasn't modified.

			current_file.info.set_attribute_boolean (PrivateAttribute.LOADED_IMAGE_WAS_MODIFIED,
				current_file.get_attribute_boolean (PrivateAttribute.LOADED_IMAGE_IS_MODIFIED));

			// The LOADED_IMAGE_IS_MODIFIED attribute must be set to false before
			// saving the file to avoid a scenario where the user is asked whether
			// he wants to save the file after saving it.

			current_file.set_is_modified (false);

			if (current_file.get_attribute_boolean (PrivateAttribute.LOADED_IMAGE_FROM_CLIPBOARD)) {
				// Ask the filename
				var read_filename = new ReadFilename (_("Save File"), _("_Save"));
				read_filename.default_value = current_file.info.get_edit_name ();
				var filename = yield read_filename.read_value (window, local_job.cancellable);
				if (filename == null) {
					throw new IOError.CANCELLED ("Cancelled");
				}

				// Save
				var file = current_file.file.get_parent ().get_child_for_display_name (filename);
				var extension = Util.get_extension (filename);
				var content_type = app.get_content_type_from_extension (extension);
				var new_file = yield save_as (file, content_type, local_job.cancellable);
				current_file.set_file (new_file);
			}
			else {
				var new_file = yield replace (local_job.cancellable);
				current_file.set_file (new_file);
			}
		}
		catch (Error error) {
			// Reset the LOADED_IMAGE_IS_MODIFIED flag if not saved.
			current_file.set_is_modified (current_file.get_attribute_boolean (PrivateAttribute.LOADED_IMAGE_WAS_MODIFIED));
			current_file.info.remove_attribute (PrivateAttribute.LOADED_IMAGE_WAS_MODIFIED);
			throw error;
		}
		finally {
			local_job.done ();
		}
	}

	async File? replace (Cancellable cancellable) throws Error {
		var current_file = window.viewer.current_file;
		var overwrite_request = OverwriteRequest.NONE;

		try {
			yield app.image_saver.replace_file (image_view.image,
					image_view.image.get_attribute ("etag"),
					current_file.file,
					current_file.get_content_type (),
					cancellable);
			return current_file.file;
		}
		catch (Error error) {
			if (error is IOError.WRONG_ETAG) {
				overwrite_request = OverwriteRequest.WRONG_ETAG;
			}
			else {
				throw error;
			}
		}

		// File changed after being loaded.
		return yield ask_to_overwrite (overwrite_request, current_file.file,
			current_file.get_content_type (), cancellable);
	}

	async File? save_as (File file, string content_type, Cancellable cancellable) throws Error {
		var overwrite_request = OverwriteRequest.NONE;

		try {
			yield app.image_saver.create_file (image_view.image, file, content_type, cancellable);
			return file;
		}
		catch (Error error) {
			if (error is IOError.EXISTS) {
				overwrite_request = OverwriteRequest.FILE_EXISTS;
			}
			else if (error is IOError.WRONG_ETAG) {
				overwrite_request = OverwriteRequest.WRONG_ETAG;
			}
			else {
				throw error;
			}
		}

		// File exists or changed after being loaded.
		return yield ask_to_overwrite (overwrite_request, file,
			content_type, cancellable);
	}

	async File? ask_to_overwrite (OverwriteRequest request, File file, string content_type, Cancellable cancellable) throws Error {
		var overwrite = new OverwriteDialog (window);
		var result = yield overwrite.ask_image (image_view.image, file, request, cancellable);
		switch (result) {
		case OverwriteResponse.CANCEL:
			throw new IOError.CANCELLED ("Cancelled");

		case OverwriteResponse.SKIP:
			break;

		case OverwriteResponse.OVERWRITE:
			yield app.image_saver.replace_file (image_view.image, null,
				file, content_type, cancellable);
			return file;

		case OverwriteResponse.RENAME:
			var parent = file.get_parent ();
			var new_file = parent.get_child_for_display_name (overwrite.new_name);
			var new_content_type = app.get_content_type_from_name (new_file);
			yield save_as (new_file, new_content_type, cancellable);
			return new_file;
		}
		return null;
	}

	void init_actions () {
		action_group = new SimpleActionGroup ();
		window.insert_action_group ("image", action_group);

		var action = new SimpleAction.stateful ("zoom-100", null, new Variant.boolean (false));
		action.activate.connect ((action, param) => {
			if (image_view.zoom_type != ZoomType.NATURAL_SIZE) {
				image_view.zoom_type = ZoomType.NATURAL_SIZE;
			}
			else {
				image_view.zoom_type = ZoomType.BEST_FIT;
			}
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("zoom-best-fit", null, new Variant.boolean (false));
		action.activate.connect ((_action, param) => {
			if (image_view.zoom_type != ZoomType.BEST_FIT) {
				image_view.zoom_type = ZoomType.BEST_FIT;
			}
			else {
				image_view.zoom_type = ZoomType.MAXIMIZE_IF_LARGER;
			}
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("zoom-view-all", null, new Variant.boolean (false));
		action.activate.connect ((_action, param) => {
			if (image_view.zoom_type != ZoomType.MAXIMIZE_IF_LARGER) {
				image_view.zoom_type = ZoomType.MAXIMIZE_IF_LARGER;
			}
			else {
				image_view.zoom_type = ZoomType.BEST_FIT;
			}
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
			case "max-size-if-larger":
				image_view.zoom_type = ZoomType.MAXIMIZE_IF_LARGER;
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

		action = new SimpleAction ("zoom-in", null);
		action.activate.connect ((_action, param) => {
			zoom_adjustment.value += zoom_adjustment.step_increment;
		});
		action_group.add_action (action);

		action = new SimpleAction ("zoom-out", null);
		action.activate.connect ((_action, param) => {
			zoom_adjustment.value -= zoom_adjustment.step_increment;
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("set-transparency", VariantType.STRING, new Variant.string (image_view.transparency.get_state ()));
		action.activate.connect ((action, param) => {
			unowned var value = param.get_string ();
			action.set_state (new Variant.string (value));
			var style = TransparencyStyle.GRAY;
			switch (value) {
			case "checkered":
				style = TransparencyStyle.CHECKERED;
				break;

			case "white":
				style = TransparencyStyle.WHITE;
				break;

			case "gray":
				style = TransparencyStyle.GRAY;
				break;

			case "black":
				style = TransparencyStyle.BLACK;
				break;
			}
			image_view.transparency = style;
		});
		action_group.add_action (action);

		action = new SimpleAction ("copy-image", null);
		action.activate.connect (() => copy_image.begin ());
		action_group.add_action (action);
	}

	async void copy_image () {
		var local_job = window.new_job ("Copy image");
		try {
			var bytes = save_png (image_view.image, null, local_job.cancellable);
			unowned var clipboard = window.get_clipboard ();
			var content_provider = new Gdk.ContentProvider.for_bytes ("image/png", bytes);
			if (!clipboard.set_content (content_provider)) {
				throw new IOError.FAILED (_("Could not copy the image to the clipboard"));
			}
			var toast = Util.new_literal_toast (_("Copied to Clipboard"));
			toast.button_label = _("Open");
			toast.action_name = "win.open-clipboard";
			window.add_toast (toast);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	void update_zoom_info () {
		if (image_view.image == null) {
			return;
		}

		// Update the statusbar.
		uint width, height;
		image_view.image.get_natural_size (out width, out height);
		zoom_info.label = "%d%%".printf ((int) Math.round (image_view.zoom * 100));

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
			case ZoomType.MAXIMIZE_IF_LARGER:
				state = "max-size-if-larger";
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

		action = action_group.lookup_action ("zoom-view-all") as SimpleAction;
		if (action != null) {
			action.set_state (new Variant.boolean (image_view.zoom_type == ZoomType.MAXIMIZE_IF_LARGER));
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

		//window.viewer.reveal_overlay_controls ();
	}

	inline double adj_to_zoom (double x) {
		return Math.exp (x / 15 - Math.E);
	}

	inline double zoom_to_adj (double z) {
		return (Math.log (z) + Math.E) * 15;
	}

	construct {
		settings = new GLib.Settings (GTHUMB_IMAGES_SCHEMA);
		settings.changed.connect ((key) => {
			switch (key) {
			case PREF_IMAGE_SCROLL_ACTION:
				scroll_action = settings.get_enum (PREF_IMAGE_SCROLL_ACTION);
				break;
			}
		});
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
	unowned Gth.ImageView image_view;
	ulong zoom_adj_changed_id;
	SimpleActionGroup action_group;
	bool dragging;
	bool clicking;
	ClickPoint drag_start;
	ScrollAction scroll_action;
	unowned Gtk.Label zoom_info;
	unowned Gtk.Adjustment zoom_adjustment;

	const float ZOOM_MIN = 0.05f;
	const float ZOOM_MAX = 10f;
	const float ZOOM_ADJ_MIN = 0f;
	const float ZOOM_ADJ_MAX = 100f;
	const double DRAG_THRESHOLD = 1;
}
