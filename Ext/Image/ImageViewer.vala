public class Gth.ImageViewer : Object, Gth.FileViewer {
	public Gth.ShortcutContext shortcut_context { get { return ShortcutContext.IMAGE_VIEWER; } }

	public void activate (Gth.Window _window) {
		assert (window == null);

		window = _window;

		typeof (Gth.ImageView).ensure ();
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/image-viewer.ui");
		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.zoom_type = (ZoomType) settings.get_enum (PREF_IMAGE_ZOOM_TYPE);
		image_view.transparency = (TransparencyStyle) settings.get_enum (PREF_IMAGE_TRANSPARENCY);
		scroll_action = (ScrollAction) settings.get_enum (PREF_IMAGE_SCROLL_ACTION);
		window.viewer.set_viewer_widget (builder.get_object ("main_view") as Gtk.Widget);
		window.viewer.set_context_menu (builder.get_object ("context_menu") as Menu);
		window.viewer.viewer_container.add_css_class ("image-view");
		window.viewer.set_left_toolbar (builder.get_object ("left_toolbar") as Gtk.Widget);
		window.viewer.set_mediabar (builder.get_object ("mediabar") as Gtk.Widget, Gtk.Align.CENTER);
		zoom_info = builder.get_object ("zoom_info") as Gtk.Label;

		var zoom_button = builder.get_object ("zoom_button") as Gtk.Widget;
		zoom_button.notify["active"].connect ((obj, spec) => {
			window.viewer.on_actived_popup (((Gtk.MenuButton) obj).active);
		});

		var options_button = builder.get_object ("options_button") as Gtk.Widget;
		options_button.notify["active"].connect ((obj, spec) => {
			window.viewer.on_actived_popup (((Gtk.MenuButton) obj).active);
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
					file_data.info.set_attribute_string (PrivateAttribute.LOADED_IMAGE_COLOR_PROFILE, image.get_attribute ("Private::ColorProfile"));
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
		var local_job = window.new_job (_("Loading %s").printf (file_data.get_display_name ()));
		load_job = local_job;
		try {
			var image = yield app.image_loader.load_file (file_data.file, LoadFlags.DEFAULT, local_job.cancellable);
			file_data.update_info (image.info, false);
			yield view_image (image, file_data, local_job.cancellable);
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
		if (image_view.image == null) {
			width = 0;
			height = 0;
			return false;
		}
		image_view.image.get_natural_size (out width, out height);
		return true;
	}

	void add_image_filters (Gtk.FileDialog dialog) {
		var filters = new ListStore (typeof (Gtk.FileFilter));
		var images_filter = new Gtk.FileFilter ();
		images_filter.name = _("Images");
		foreach (unowned var content_type in app.savers.get_keys ()) {
			images_filter.add_mime_type (content_type);
			var type_filter = new Gtk.FileFilter ();
			type_filter.add_mime_type (content_type);
			var saver = app.get_saver_preferences (content_type);
			type_filter.name = saver.get_display_name ();
			filters.append (type_filter);
		}
		filters.insert (0, images_filter);
		dialog.filters = filters;
		dialog.default_filter = images_filter;
	}

	public async void ask_name_and_save () {
		var dialog = new Gtk.FileDialog ();
		dialog.initial_folder = window.viewer.current_file.file.get_parent ();
		dialog.initial_name = window.viewer.current_file.get_display_name ();
		add_image_filters (dialog);
		try {
			var file = yield dialog.save (window, null);
			var file_data = yield save_to (file);
			window.viewer.file_saved (file_data);
		}
		catch (Error error) {
			window.show_error (error);
		}
	}

	async FileData? save_to (File file) throws Error {
		var new_filename = !file.equal (window.viewer.current_file.file);
		var file_data = new FileData.for_rename (window.viewer.current_file, file);
		var local_job = window.new_job (_("Saving %s").printf (file_data.get_display_name ()),
			JobFlags.FOREGROUND,
			"document-save-symbolic");
		try {
			// TODO: load the original image if a lower quality version was loaded.

			// Save LOADED_IMAGE_IS_MODIFIED into LOADED_IMAGE_WAS_MODIFIED
			// to allow the exiv2 metadata writer to not change some fields if the
			// content wasn't modified.

			file_data.info.set_attribute_boolean (PrivateAttribute.LOADED_IMAGE_WAS_MODIFIED,
				file_data.get_attribute_boolean (PrivateAttribute.LOADED_IMAGE_IS_MODIFIED));

			// The LOADED_IMAGE_IS_MODIFIED attribute must be set to false before
			// saving the file to avoid a scenario where the user is asked whether
			// he wants to save the file after saving it.

			file_data.set_is_modified (false);

			if (file_data.get_attribute_boolean (PrivateAttribute.LOADED_IMAGE_FROM_CLIPBOARD)) {
				// Ask the filename
				var read_filename = new ReadFilename (_("Save File"), _("_Save"));
				read_filename.default_value = file_data.info.get_edit_name ();
				var basename = yield read_filename.read_value (window, local_job);
				file_data.rename_from_display_name (basename);
				new_filename = true;
			}

			// Allow to change the format options before saving
			if (app.settings.get_boolean (PREF_GENERAL_SHOW_FORMAT_OPTIONS)) {
				var dialog = new FormatPreferencesDialog ();
				yield dialog.change_options (window, file_data.get_content_type (), local_job);
			}

			// Save
			if (new_filename) {
				file_data = yield create_file (file_data, local_job);
			}
			else {
				file_data = yield replace_file (file_data, local_job);
			}
		}
		catch (Error error) {
			// Reset the LOADED_IMAGE_IS_MODIFIED flag if not saved.
			file_data.set_is_modified (file_data.get_attribute_boolean (PrivateAttribute.LOADED_IMAGE_WAS_MODIFIED));
			file_data.info.remove_attribute (PrivateAttribute.LOADED_IMAGE_WAS_MODIFIED);
			throw error;
		}
		finally {
			local_job.done ();
		}
		return file_data;
	}

	public override async void save () throws Error {
		window.viewer.current_file = yield save_to (window.viewer.current_file.file);
	}

	public void focus () {
		image_view.grab_focus ();
	}

	async FileData? replace_file (FileData file_data, Job job) throws Error {
		var overwrite_request = OverwriteRequest.NONE;
		try {
			yield app.image_saver.replace_file (image_view.image, file_data, SaveFlags.DEFAULT, job.cancellable);
			return file_data;
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
		return yield ask_to_overwrite (overwrite_request, file_data, job);
	}

	async FileData? create_file (FileData file_data, Job job) throws Error {
		var overwrite_request = OverwriteRequest.NONE;
		try {
			yield app.image_saver.create_file (image_view.image, file_data, SaveFlags.DEFAULT, job.cancellable);
			return file_data;
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
		return yield ask_to_overwrite (overwrite_request, file_data, job);
	}

	async FileData? ask_to_overwrite (OverwriteRequest request, FileData file_data, Job job) throws Error {
		var overwrite = new OverwriteDialog (window);
		overwrite.check_extension = true;
		var result = yield overwrite.ask_image (image_view.image, file_data.file, request, job);
		switch (result) {
		case OverwriteResponse.CANCEL:
			throw new IOError.CANCELLED ("Cancelled");

		case OverwriteResponse.OVERWRITE:
			file_data.set_etag (null);
			yield app.image_saver.replace_file (image_view.image, file_data, SaveFlags.DEFAULT, job.cancellable);
			return file_data;

		case OverwriteResponse.RENAME:
			file_data.rename_from_display_name (overwrite.new_name);
			return yield create_file (file_data, job);

		default:
			break;
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

		action = new SimpleAction ("save-as", null);
		action.activate.connect (() => ask_name_and_save.begin ());
		action_group.add_action (action);

		action = new SimpleAction ("flip-horizontal", null);
		action.activate.connect (() => {
			edit_image.begin (_("Horizontal Flip"), (image, cancellable) => {
				return image.apply_transform (Gth.Transform.FLIP_H, cancellable);
			});
		});
		action_group.add_action (action);

		action = new SimpleAction ("flip-vertical", null);
		action.activate.connect (() => {
			edit_image.begin (_("Vertical Flip"), (image, cancellable) => {
				return image.apply_transform (Gth.Transform.FLIP_V, cancellable);
			});
		});
		action_group.add_action (action);

		action = new SimpleAction ("rotate-right", null);
		action.activate.connect (() => {
			edit_image.begin (_("Rotate Right"), (image, cancellable) => {
				return image.apply_transform (Gth.Transform.ROTATE_90, cancellable);
			});
		});
		action_group.add_action (action);

		action = new SimpleAction ("rotate-left", null);
		action.activate.connect (() => {
			edit_image.begin (_("Rotate Left"), (image, cancellable) => {
				return image.apply_transform (Gth.Transform.ROTATE_270, cancellable);
			});
		});
		action_group.add_action (action);
	}

	async void edit_image (string title, EditorFunc editor_func) {
		var local_job = window.new_job (title, JobFlags.FOREGROUND);
		try {
			// TODO: load the original image if needed.
			image_view.image = yield app.image_editor.exec_async (
				image_view.image,
				local_job.cancellable,
				editor_func);
			window.viewer.current_file.set_is_modified (true);
			window.viewer.update_title ();
			// TODO window.viewer.update_sensitivity ();
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
		}
	}

	async void copy_image () {
		var local_job = window.new_job (_("Copying to the Clipboard"), JobFlags.FOREGROUND);
		try {
			var bytes = save_png (image_view.image, null, local_job.cancellable);
			unowned var clipboard = window.get_clipboard ();
			var content_provider = new Gdk.ContentProvider.for_bytes ("image/png", bytes);
			if (!clipboard.set_content (content_provider)) {
				throw new IOError.FAILED ("Operation failed");
			}
			var toast = Util.new_literal_toast (_("Copied to the Clipboard"));
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
				scroll_action = (ScrollAction) settings.get_enum (PREF_IMAGE_SCROLL_ACTION);
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
