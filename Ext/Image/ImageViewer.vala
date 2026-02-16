public class Gth.ImageViewer : Object, Gth.FileViewer {
	public Gth.ShortcutContext shortcut_context { get { return ShortcutContext.IMAGE_VIEWER; } }

	public void activate (Gth.Window _window) {
		assert (window == null);

		window = _window;

		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/image-viewer.ui");
		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.zoom_type = (ZoomType) settings.get_enum (PREF_IMAGE_ZOOM_TYPE);
		image_view.transparency = (TransparencyStyle) settings.get_enum (PREF_IMAGE_TRANSPARENCY);
		animation_actions = builder.get_object ("animation_actions") as Gtk.Box;
		scroll_action = (ScrollAction) settings.get_enum (PREF_IMAGE_SCROLL_ACTION);
		apply_icc_profile = settings.get_boolean (PREF_IMAGE_APPLY_ICC_PROFILE);
		window.viewer.set_viewer_widget (builder.get_object ("main_view") as Gtk.Widget);
		window.viewer.viewer_container.add_css_class ("image-view");
		window.viewer.set_left_toolbar (builder.get_object ("left_toolbar") as Gtk.Widget);
		window.viewer.set_mediabar (builder.get_object ("mediabar") as Gtk.Widget, Gtk.Align.CENTER);
		zoom_info = builder.get_object ("zoom_info") as Gtk.Label;

		unowned var overview_button = builder.get_object ("overview_button") as Gtk.MenuButton;
		overview_button.notify["active"].connect ((obj, spec) => {
			var local_button = obj as Gtk.MenuButton;
			if (local_button.active) {
				var overview = builder.get_object ("overview") as Gth.ImageOverview;
				overview.main_view = image_view;
			}
			window.viewer.on_actived_popup (local_button.active);
		});

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
			image_view.zoom = ZoomScale.get_zoom (local_adj.get_value (), image_view.min_zoom, image_view.max_zoom);
		});

		image_view.resized.connect (() => update_zoom_info ());
		image_view.add_drag_controller ();

		var scroll_events = new Gtk.EventControllerScroll (Gtk.EventControllerScrollFlags.VERTICAL);
		scroll_events.scroll.connect ((controller, dx, dy) => {
			return on_scroll (dx, dy, controller.get_current_event_state ());
		});
		image_view.add_controller (scroll_events);

		var key_events = new Gtk.EventControllerKey ();
		key_events.key_pressed.connect (window.on_key_pressed);
		image_view.add_controller (key_events);

		init_actions ();
	}

	async void view_image (Gth.Image image, Gth.FileData? file_data, Cancellable cancellable) {
		// Keep the previous zoom type.
		image_view.default_zoom_type = image_view.zoom_type;
		image_view.image = image;
	}

	public async bool load (FileData file_data, Job job) {
		var success = false;
		preloader.cancel ();
		if (window.viewer.current_file != null) {
			preloader.cache.touch (window.viewer.current_file.file);
		}
		try {
			var flags = LoadFlags.DEFAULT;
			if (!apply_icc_profile) {
				flags |= LoadFlags.NO_ICC_PROFILE;
			}
			var image = preloader.cache[file_data.file];
			if (image == null) {
				// stdout.printf ("> LOAD: %s\n", file_data.get_display_name ());
				image = yield app.image_loader.load_file (window, file_data.file, flags, job.cancellable);
			}
			else {
				// stdout.printf ("> FROM CACHE: %s\n", file_data.get_display_name ());
			}
			file_data.update_info (image.info, false);
			yield view_image (image, file_data, job.cancellable);
			history.clear ();
			history.add (image, false);
			if (apply_icc_profile) {
				preloader.cache.add (file_data.file, image);
			}
			success = true;
		}
		catch (Error error) {
			if (error is IOError.CANCELLED) {
				return false;
			}
			window.show_error (error);
			image_view.image = null;
		}
		return success;
	}

	public override void preload_some_files () {
		preloader.cancel ();
		var queue = new Queue<File>();

		void add_to_queue (uint pos) {
			var file_data = window.browser.file_grid.model.get_item (pos) as FileData;
			if (file_data == null) {
				return;
			}
			if (!app.viewer_can_view (this.get_class ().get_type (), file_data.get_content_type ())) {
				return;
			}
			if (file_data.file in preloader.cache) {
				return;
			}
			// stdout.printf ("> ADD TO QUEUE: %s\n", file_data.file.get_uri ());
			queue.push_tail (file_data.file);
		}

		add_to_queue (window.viewer.position + 1);
		if (window.viewer.position > 0) {
			add_to_queue (window.viewer.position - 1);
		}
		if (queue.is_empty ()) {
			return;
		}
		var local_job = window.new_job ("Preload", JobFlags.HIDDEN);
		preloader.load.begin (window, queue, local_job, 0, (_obj, res) => {
			preloader.load.end (res);
			// preloader.cache.print ();
			local_job.done ();
		});
	}

	public async void view_unsaved_image (Gth.Image image, Gth.FileData? file_data, Cancellable cancellable) {
		yield view_image (image, file_data, cancellable);
		history.clear ();
		history.add (image, true);
	}

	public void deactivate () {
		if (edit_job != null) {
			edit_job.cancel ();
		}
		if (preloader != null) {
			preloader.cancel ();
		}
		window.viewer.viewer_container.remove_css_class ("image-view");
		window.insert_action_group ("viewer", null);
		window.insert_action_group ("image", null);
	}

	public void after_closing_editor () {
		// Restore the image.* actions overwritten by the Editor.
		window.insert_action_group ("image", image_view.action_group);
	}

	public void save_preferences () {
		settings.set_enum (PREF_IMAGE_ZOOM_TYPE, image_view.zoom_type);
		settings.set_enum (PREF_IMAGE_TRANSPARENCY, image_view.transparency);
		settings.set_boolean (PREF_IMAGE_APPLY_ICC_PROFILE, apply_icc_profile);
	}

	public void release_resources () {
		history.clear ();
		builder = null;
	}

	public bool on_scroll (double dx, double dy, Gdk.ModifierType state) {
		var action = scroll_action;
		if ((state & Gdk.ModifierType.CONTROL_MASK) != 0) {
			action = ScrollAction.CHANGE_ZOOM;
		}
		switch (action) {
		case ScrollAction.CHANGE_FILE:
			return window.viewer.on_scroll_change_file (dx, dy);
		case ScrollAction.CHANGE_ZOOM:
			return image_view.on_scroll (dx, dy, state);
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


	public async void ask_name_and_save () {
		File new_file = null;
		var local_job = window.new_job ("Ask Name");
		try {
			var default_value = window.viewer.current_file.info.get_edit_name ();
			var last_extension = app.settings.get_string (PREF_BROWSER_LAST_EXTENSION);
			if (!Strings.empty (last_extension)) {
				var default_value_no_ext = Util.remove_extension (default_value);
				default_value = default_value_no_ext + "." + last_extension;
			}

			var read_filename = new ReadFilename (_("Save File"), _("_Save"));
			read_filename.default_value = default_value;
			read_filename.check_extension = true;
			var basename = yield read_filename.read_value (window, local_job);

			var ext = Util.get_extension (basename);
			app.settings.set_string (PREF_BROWSER_LAST_EXTENSION, ext);

			var folder = window.viewer.current_file.file.get_parent ();
			new_file = folder.get_child_for_display_name (basename);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
		}

		if (new_file == null) {
			return;
		}

		try {
			window.viewer.current_file.info.set_attribute_boolean (PrivateAttribute.ASK_FILENAME_WHEN_SAVING, false);
			var file_data = yield save_to (new_file);
			window.viewer.after_saving (file_data);
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
			// Save LOADED_IMAGE_IS_MODIFIED into LOADED_IMAGE_WAS_MODIFIED
			// to allow the exiv2 metadata writer to not change some fields if the
			// content wasn't modified.

			file_data.info.set_attribute_boolean (PrivateAttribute.LOADED_IMAGE_WAS_MODIFIED,
				file_data.get_attribute_boolean (PrivateAttribute.LOADED_IMAGE_IS_MODIFIED));

			// The LOADED_IMAGE_IS_MODIFIED attribute must be set to false before
			// saving the file to avoid a scenario where the user is asked whether
			// he wants to save the file after saving it.

			file_data.set_is_modified (false);

			if (file_data.get_attribute_boolean (PrivateAttribute.ASK_FILENAME_WHEN_SAVING)
				|| !app.savers.contains (file_data.get_content_type ()))
			{
				// Ask the filename
				var read_filename = new ReadFilename (_("Save File"), _("_Save"));
				read_filename.default_value = file_data.info.get_edit_name ();
				read_filename.check_extension = true;
				var basename = yield read_filename.read_value (window, local_job);
				file_data.rename_from_display_name (basename);
				file_data.info.set_attribute_boolean (PrivateAttribute.ASK_FILENAME_WHEN_SAVING, false);
				new_filename = true;

				// Save the file type as the default clipboard type.
				var saver = app.get_saver_preferences (file_data.get_content_type ());
				if (saver == null) {
					throw new IOError.FAILED (_("File type not supported"));
				}
				app.settings.set_string (PREF_BROWSER_CLIPBOARD_TYPE, saver.get_content_type ());
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
			history.current_was_saved ();
		}
		return file_data;
	}

	public override async void save () throws Error {
		if (window.viewer.current_file == null) {
			throw new IOError.FAILED ("No file");
		}
		var file_data = yield save_to (window.viewer.current_file.file);
		window.viewer.after_saving (file_data);
	}

	public override bool same_etag (FileInfo info) {
		return (image_view != null)
			&& (image_view.image != null)
			&& (image_view.image.info != null)
			&& Files.same_etag (image_view.image.info, info);
	}

	public void revert_to_saved () {
		set_other_version (history.revert (), false);
	}

	public void focus () {
		image_view.grab_focus ();
	}

	async FileData? replace_file (FileData file_data, Job job) throws Error {
		var overwrite_request = OverwriteRequest.NONE;
		try {
			yield app.image_saver.replace_file (window, image_view.image, file_data, SaveFlags.DEFAULT, job.cancellable);
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
			yield app.image_saver.create_file (window, image_view.image, file_data, SaveFlags.DEFAULT, job.cancellable);
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
			yield app.image_saver.replace_file (window, image_view.image, file_data, SaveFlags.DEFAULT, job.cancellable);
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
		window.insert_action_group ("image", image_view.action_group);

		action_group = new SimpleActionGroup ();
		window.insert_action_group ("viewer", action_group);

		var action = new SimpleAction ("copy-image", null);
		action.activate.connect (() => copy_image.begin ());
		action_group.add_action (action);

		action = new SimpleAction ("save-as", null);
		action.activate.connect (() => ask_name_and_save.begin ());
		action_group.add_action (action);

		action = new SimpleAction ("save", null);
		action.activate.connect (() => save.begin ());
		action_group.add_action (action);

		action = new SimpleAction ("revert", null);
		action.activate.connect (() => revert_to_saved ());
		action_group.add_action (action);

		action = new SimpleAction ("flip-horizontal", null);
		action.activate.connect (() => {
			if (image_view.image == null) {
				return;
			}
			edit_image.begin (_("Horizontal Flip"), new ImageTransform (Transform.FLIP_H));
		});
		action_group.add_action (action);

		action = new SimpleAction ("flip-vertical", null);
		action.activate.connect (() => {
			if (image_view.image == null) {
				return;
			}
			edit_image.begin (_("Vertical Flip"), new ImageTransform (Transform.FLIP_V));
		});
		action_group.add_action (action);

		action = new SimpleAction ("rotate-right", null);
		action.activate.connect (() => {
			if (image_view.image == null) {
				return;
			}
			edit_image.begin (_("Rotate Right"), new ImageTransform (Transform.ROTATE_90));
		});
		action_group.add_action (action);

		action = new SimpleAction ("rotate-left", null);
		action.activate.connect (() => {
			if (image_view.image == null) {
				return;
			}
			edit_image.begin (_("Rotate Right"), new ImageTransform (Transform.ROTATE_270));
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("apply-icc-profile", null, new Variant.boolean (true));
		action.activate.connect ((action, param) => {
			apply_icc_profile = Util.toggle_state (action);
			window.viewer.reload ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("undo", null);
		action.activate.connect ((action, param) => {
			bool is_modified;
			var image = history.undo (out is_modified);
			if (image != null) {
				set_other_version (image, is_modified);
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("redo", null);
		action.activate.connect ((action, param) => {
			bool is_modified;
			var image = history.redo (out is_modified);
			if (image != null) {
				set_other_version (image, is_modified);
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("color-picker", null);
		action.activate.connect ((action, param) => {
			window.editor.activate_tool (new ColorPicker ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("adjust-contrast", null);
		action.activate.connect ((action, param) => {
			window.editor.activate_tool (new AdjustContrast ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("grayscale", null);
		action.activate.connect ((action, param) => {
			window.editor.activate_tool (new Grayscale ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("special-effects", null);
		action.activate.connect ((action, param) => {
			window.editor.activate_tool (new SpecialEffects ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("adjust-saturation", null);
		action.activate.connect ((action, param) => {
			window.editor.activate_tool (new AdjustSaturation ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("adjust-brightness", null);
		action.activate.connect ((action, param) => {
			window.editor.activate_tool (new AdjustBrightness ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("sharpen-image", null);
		action.activate.connect ((action, param) => {
			window.editor.activate_tool (new SharpenImage ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("crop", null);
		action.activate.connect ((action, param) => {
			window.editor.activate_tool (new Crop ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("adjust-colors", null);
		action.activate.connect ((action, param) => {
			window.editor.activate_tool (new AdjustColors ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("resize", null);
		action.activate.connect ((action, param) => {
			window.editor.activate_tool (new ResizeImage ());
		});
		action_group.add_action (action);

		action = new SimpleAction ("rotate", null);
		action.activate.connect ((action, param) => {
			window.editor.activate_tool (new RotateImage ());
		});
		action_group.add_action (action);
	}

	public async bool edit_image (string title, ImageOperation operation, string? icon_name = null) {
		if (edit_job != null) {
			edit_job.cancel ();
		}
		var edited = false;
		var local_job = window.new_job (title, JobFlags.FOREGROUND, icon_name);
		edit_job = local_job;
		try {
			// TODO: load the original image if needed.
			var image = yield app.image_editor.exec_operation (
				image_view.image,
				operation,
				local_job.cancellable);
			history.add (image, true);
			set_other_version (image, true);
			edited = true;
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			local_job.done ();
			if (local_job == edit_job) {
				edit_job = null;
			}
		}
		return edited;
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
		var action = image_view.action_group.lookup_action ("set-zoom") as SimpleAction;
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

		action = image_view.action_group.lookup_action ("zoom-100") as SimpleAction;
		if (action != null) {
			var active = image_view.zoom_type == ZoomType.NATURAL_SIZE;
			action.set_state (new Variant.boolean (active));
			var button = builder.get_object ("natural_size_button") as Gtk.ToggleButton;
			button.active = active;
		}

		action = image_view.action_group.lookup_action ("zoom-best-fit") as SimpleAction;
		if (action != null) {
			action.set_state (new Variant.boolean (image_view.zoom_type == ZoomType.BEST_FIT));
		}

		action = image_view.action_group.lookup_action ("zoom-view-all") as SimpleAction;
		if (action != null) {
			var active = image_view.zoom_type == ZoomType.MAXIMIZE_IF_LARGER;
			action.set_state (new Variant.boolean (active));
			var button = builder.get_object ("view_all_button") as Gtk.ToggleButton;
			button.active = active;
		}

		// Update the zoom adjustment.
		unowned var adj = builder.get_object ("zoom_adjustment") as Gtk.Adjustment;
		SignalHandler.block (adj, zoom_adj_changed_id);
		adj.set_value (ZoomScale.get_adj_value (image_view.zoom, image_view.min_zoom, image_view.max_zoom));
		SignalHandler.unblock (adj, zoom_adj_changed_id);
	}

	void update_sensitivity () {
		var has_image = (window.viewer.current_file != null) && (image_view.image != null) && !image_view.image.get_is_empty ();
		var is_modified = has_image && window.viewer.current_file.get_is_modified ();
		Util.enable_action (action_group, "apply-icc-profile", has_image && image_view.image.has_icc_profile () && !is_modified);
		var can_save_format = has_image && app.savers.contains (window.viewer.current_file.get_content_type ());
		Util.enable_action (action_group, "save", has_image && can_save_format && is_modified);
		Util.enable_action (action_group, "save-as", has_image);

		// Toolbar actions
		animation_actions.visible = (image_view.image != null) && image_view.image.get_is_animated ();
	}

	void set_other_version (Image image, bool is_modified) {
		// Set a new version without changing the zoom type.
		var default_zoom_type = image_view.default_zoom_type;
		image_view.default_zoom_type = ZoomType.KEEP_PREVIOUS;
		image_view.image = image;
		image_view.default_zoom_type = default_zoom_type;

		window.viewer.current_file.set_is_modified (is_modified);
		Lib.backup_attribute (window.viewer.current_file.info, "Frame::Size");
		Lib.backup_attribute (window.viewer.current_file.info, "Frame::Width");
		Lib.backup_attribute (window.viewer.current_file.info, "Frame::Height");
		Lib.set_frame_size (window.viewer.current_file.info, (int) image.width, (int) image.height);
		window.viewer.update_title ();
		window.viewer.property_sidebar.update_view ();
		update_sensitivity ();
		focus ();
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
		history = new ImageHistory ();
		history.changed.connect (() => {
			Util.enable_action (action_group, "undo", history.can_undo);
			Util.enable_action (action_group, "redo", history.can_redo);
		});
		preloader = new Preloader ();
	}

	weak Gth.Window window;
	GLib.Settings settings;
	Gtk.Builder builder;
	public unowned Gth.ImageView image_view;
	unowned Gtk.Box animation_actions;
	ulong zoom_adj_changed_id;
	SimpleActionGroup action_group;
	ScrollAction scroll_action;
	unowned Gtk.Label zoom_info;
	unowned Gtk.Adjustment zoom_adjustment;
	bool apply_icc_profile;
	ImageHistory history;
	Job edit_job = null;
	Preloader preloader;
}

class Gth.ImageTransform : Gth.ImageOperation {
	public Transform transform;

	public ImageTransform (Transform _transform) {
		transform = _transform;
	}

	public override Gth.Image? execute (Image image, Cancellable cancellable) {
		return image.apply_transform (transform, cancellable);
	}
}
