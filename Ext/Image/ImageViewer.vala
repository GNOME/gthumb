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
			image_view.zoom = ZoomScale.get_zoom (local_adj.get_value ());
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
		try {
			// TODO: make sure to apply only once?
			var icc_profile = apply_icc_profile ? image.get_icc_profile () : null;
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

	public async bool load (FileData file_data, Job job) {
		var success = false;
		try {
			var image = yield app.image_loader.load_file (file_data.file, LoadFlags.DEFAULT, job.cancellable);
			file_data.update_info (image.info, false);
			yield view_image (image, file_data, job.cancellable);
			history.clear ();
			history.add (image, false);
			success = true;
		}
		catch (Error error) {
			if (error is IOError.CANCELLED) {
				return false;
			}
			stdout.printf ("> ERROR: %s\n", error.message);
			window.show_error (error);
			image_view.image = null;
		}
		update_toolbar_actions ();
		update_sensitivity ();
		return success;
	}

	public async void view_unsaved_image (Gth.Image image, Gth.FileData? file_data, Cancellable cancellable) {
		yield view_image (image, file_data, cancellable);
		history.clear ();
		history.add (image, true);
		update_toolbar_actions ();
		update_sensitivity ();
	}

	public void deactivate () {
		if (edit_job != null) {
			edit_job.cancel ();
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
			history.current_was_saved ();
		}
		return file_data;
	}

	public override async void save () throws Error {
		window.viewer.current_file = yield save_to (window.viewer.current_file.file);
	}

	public void revert_to_saved () {
		set_image (history.revert (), false);
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
				set_image (image, is_modified);
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("redo", null);
		action.activate.connect ((action, param) => {
			bool is_modified;
			var image = history.redo (out is_modified);
			if (image != null) {
				set_image (image, is_modified);
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
			set_image (image, true);
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

	void update_toolbar_actions () {
		animation_actions.visible = (image_view.image != null) && image_view.image.get_is_animated ();
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
		adj.set_value (ZoomScale.get_adj_value (image_view.zoom));
		SignalHandler.unblock (adj, zoom_adj_changed_id);
	}

	void update_sensitivity () {
		var has_image = (window.viewer.current_file != null) && (image_view.image != null) && !image_view.image.get_is_empty ();
		var is_modified = has_image && window.viewer.current_file.get_is_modified ();
		Util.enable_action (action_group, "apply-icc-profile", has_image && image_view.image.has_icc_profile () && !is_modified);
		Util.enable_action (action_group, "save", has_image && is_modified);
	}

	void set_image (Image image, bool is_modified) {
		image_view.image = image;
		window.viewer.current_file.set_is_modified (is_modified);
		window.viewer.update_title ();
		update_sensitivity ();
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
}

class Gth.ImageTransform : Gth.ImageOperation {
	public Transform tranform;

	public ImageTransform (Transform _tranform) {
		tranform = _tranform;
	}

	public override Gth.Image? execute (Image image, Cancellable cancellable) {
		return image.apply_transform (tranform, cancellable);
	}
}
