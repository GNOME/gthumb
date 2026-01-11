public class Gth.ResizeImage : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/resize-image.ui");
		window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.image = original;
		add_default_controllers (image_view);
		image_view.image = viewer.image_view.image;
		image_view.set_first_state_from_view (viewer.image_view);

		window.editor.set_content (image_view);

		var high_quality_switch = builder.get_object ("high_quality_switch") as Adw.SwitchRow;
		high_quality_switch.notify["active"].connect ((obj, _param) => {
			var local_switch = obj as Adw.SwitchRow;
			high_quality = local_switch.active;
			queue_update_size ();
		});

		zoom_adjustment = builder.get_object ("zoom_adjustment") as Gtk.Adjustment;
		zoom_adj_changed_id = zoom_adjustment.value_changed.connect ((local_adj) => {
			var scale_factor = local_adj.get_value () / 100.0; //ZoomScale.get_zoom (local_adj.get_value (), MIN_ZOOM, MAX_ZOOM);
			var new_width = (uint) (scale_factor * original.width);
			var new_height = (uint) (scale_factor * original.height);
			set_size (new_width, new_height);
		});

		var reset_button = builder.get_object ("reset_button") as Gtk.Button;
		reset_button.clicked.connect (() => reset_size ());

		var spin_row = builder.get_object ("width") as Adw.SpinRow;
		width_changed_id = spin_row.adjustment.value_changed.connect ((obj) => {
			var local_adj = obj as Gtk.Adjustment;
			uint new_width, new_height;
			if (unit == ResizeUnit.PERCENT) {
				new_width = (uint) (local_adj.value * original.width / 100.0);
				new_height = (ratio == 0) ? height : (uint) (local_adj.value * original.height / 100.0);
			}
			else {
				new_width = (uint) local_adj.value;
				new_height = (ratio == 0) ? height : (uint) ((float) new_width / ratio);
			}
			set_size (new_width, new_height);
		});

		spin_row = builder.get_object ("height") as Adw.SpinRow;
		height_changed_id = spin_row.adjustment.value_changed.connect ((obj) => {
			var local_adj = obj as Gtk.Adjustment;
			uint new_width, new_height;
			if (unit == ResizeUnit.PERCENT) {
				new_height = (uint) (local_adj.value * original.height / 100.0);
				new_width = (ratio == 0) ? width : (uint) (local_adj.value * original.width / 100.0);
			}
			else {
				new_height = (uint) local_adj.value;
				new_width = (ratio == 0) ? width : (uint) (ratio * new_height);
			}
			set_size (new_width, new_height);
		});

		var special_size_group = builder.get_object ("special_size_group") as Adw.PreferencesGroup;
		var display = window.root.get_display ();
		var monitors = display.get_monitors ();
		var n_monitors = monitors.get_n_items ();
		for (var i = 0; i < n_monitors; i++) {
			var monitor = monitors.get_item (i) as Gdk.Monitor;
			var geometry = monitor.geometry;
			var button = new Adw.ButtonRow ();
			button.child = new_size_button_content (_("Screen"), (uint) geometry.width, (uint) geometry.height);
			button.activated.connect (() => {
				if (ratio == 0) {
					set_size ((uint) geometry.width, (uint) geometry.height);
				}
				else {
					// Make the image fit the screen size respecting the aspect ratio.
					var new_width = (uint) geometry.width;
					var new_height = (uint) ((float) new_width / ratio);
					if (new_height > geometry.height) {
						new_height = (uint) geometry.height;
						new_width = (uint) ((float) new_height * ratio);
					}
					set_size (new_width, new_height);
				}
			});
			special_size_group.add (button);
		}

		void add_special_size (string display_name, uint width, uint height) {
			var button = new Adw.ButtonRow ();
			button.child = new_size_button_content (display_name, width, height);
			button.activated.connect (() => set_size_with_ratio (width, height));
			special_size_group.add (button);
		}

		add_special_size (_("4K"), 3840, 2160);
		add_special_size (_("1080p"), 1920, 1080);
		add_special_size (_("720p"), 1280, 720);

		var more_options = builder.get_object ("more_options") as Gtk.Button;
		more_options.clicked.connect (() => {
			var page = builder.get_object ("more_options_page") as Adw.NavigationPage;
			window.editor.sidebar.push (page);
		});

		var aspect_ratio_group = builder.get_object ("aspect_ratio_group") as Gth.AspectRatioGroup;
		aspect_ratio_group.activate (window, original, true);
		aspect_ratio_group.changed.connect ((local_group, after_rotation) => {
			update_ratio (after_rotation);
		});

		var unit_group = builder.get_object ("unit_group") as Adw.ToggleGroup;
		unit_group.active_name = (unit == ResizeUnit.PIXEL) ? "pixels" : "percent";
		unit_group.notify["active-name"].connect ((obj, param) => {
			var local_group = obj as Adw.ToggleGroup;
			unit = (local_group.active_name == "pixels") ? ResizeUnit.PIXEL : ResizeUnit.PERCENT;
			set_size (width, height);
		});

		ratio = (float) original.width / original.height;
		set_size (original.width, original.height);
	}

	public override void before_deactivate () {
		builder = null;
		if (preview_job != null) {
			preview_job.cancel ();
		}
		if (update_id != 0) {
			Source.remove (update_id);
			update_id = 0;
		}
	}

	public override ImageOperation? get_operation () {
		return new ResizeOperation (width, height, high_quality);
	}

	Gtk.Widget new_size_button_content (string title, uint width, uint height, string? icon = null) {
		var box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 6);
		if (icon == null) {
			icon = (width > height) ? "gth-page-orientation-landscape-symbolic"
				: (width < height) ? "gth-page-orientation-portrait-symbolic"
				: "gth-page-orientation-squared-symbolic";
		}
		box.append (new Gtk.Image.from_icon_name (icon));
		var title_label = new Gtk.Label (title);
		title_label.hexpand = true;
		title_label.xalign = 0;
		title_label.add_css_class ("button-label");
		box.append (title_label);
		var size_label = new Gtk.Label ("%u × %u".printf (width, height));
		size_label.add_css_class ("dimmed");
		box.append (size_label);
		return box;
	}

	void reset_size (double zoom = 1.0) {
		var aspect_ratio_group = builder.get_object ("aspect_ratio_group") as Gth.AspectRatioGroup;
		aspect_ratio_group.set_image_ratio ();
		set_size ((uint) Math.round (original.width * zoom), (uint) Math.round (original.height * zoom));
	}

	construct {
		title = _("Resize");
		icon_name = "gth-resize-symbolic";
		preview_job = null;
		unit = ResizeUnit.PIXEL;
	}

	void set_size (uint _width, uint _height) {
		width = _width;
		height = _height;

		SignalHandler.block (zoom_adjustment, zoom_adj_changed_id);
		// zoom_adjustment.value = ZoomScale.get_adj_value ((float) width / original.width, MIN_ZOOM, MAX_ZOOM);
		var percent = (double) width / original.width * 100;
		zoom_adjustment.upper = (percent > DEFAULT_MAX_SCALE_VALUE) ? percent : DEFAULT_MAX_SCALE_VALUE;
		zoom_adjustment.value = percent;
		SignalHandler.unblock (zoom_adjustment, zoom_adj_changed_id);

		var spin_row = builder.get_object ("width") as Adw.SpinRow;
		SignalHandler.block (spin_row.adjustment, width_changed_id);
		spin_row.adjustment.freeze_notify ();
		if (unit == ResizeUnit.PERCENT) {
			spin_row.adjustment.lower = 1;
			spin_row.adjustment.upper = MAX_PERCENT;
			spin_row.adjustment.step_increment = 1;
			spin_row.adjustment.value = (double) width / original.width * 100;;
		}
		else {
			spin_row.adjustment.lower = 1;
			spin_row.adjustment.upper = MAX_SIZE;
			spin_row.adjustment.step_increment = 1;
			spin_row.adjustment.value = (double) width;
		}
		spin_row.adjustment.thaw_notify ();
		SignalHandler.unblock (spin_row.adjustment, width_changed_id);

		spin_row = builder.get_object ("height") as Adw.SpinRow;
		SignalHandler.block (spin_row.adjustment, height_changed_id);
		spin_row.adjustment.freeze_notify ();
		if (unit == ResizeUnit.PERCENT) {
			spin_row.adjustment.lower = 1;
			spin_row.adjustment.upper = MAX_PERCENT;
			spin_row.adjustment.step_increment = 1;
			spin_row.adjustment.value = (double) height / original.height * 100;
		}
		else {
			spin_row.adjustment.lower = 1;
			spin_row.adjustment.upper = MAX_SIZE;
			spin_row.adjustment.step_increment = 1;
			spin_row.adjustment.value = (double) height;
		}
		spin_row.adjustment.thaw_notify ();
		SignalHandler.unblock (spin_row.adjustment, height_changed_id);

		queue_update_size ();
	}

	void set_size_with_ratio (uint width, uint height) {
		if (ratio != 0) {
			var new_height = height;
			var new_width = (uint) (ratio * height);
			if (new_width > width) {
				new_width = width;
				new_height = (uint) ((float) new_width / ratio);
			}
			width = new_width;
			height = new_height;
		}
		set_size (width, height);
	}

	void update_ratio (bool swap_size) {
		var aspect_ratio_group = builder.get_object ("aspect_ratio_group") as Gth.AspectRatioGroup;
		ratio = aspect_ratio_group.ratio;
		if (ratio != 0) {
			set_size (width, (uint) ((float) width / ratio));
		}
	}

	uint update_id = 0;

	void queue_update_size () {
		if (update_id != 0) {
			return;
		}
		update_id = Util.after_timeout (UPDATE_DELAY, () => {
			update_id = 0;
			update_size ();
		});
	}

	void update_size () {
		// stdout.printf ("> RESIZE: %u,%u\n", width, height);
		if (preview_job != null) {
			preview_job.cancel ();
		}
		var job = window.new_job ("Update Preview");
		preview_job = job;
		var operation = get_operation ();
		app.image_editor.exec_operation.begin (original, operation, job.cancellable, (_obj, res) => {
			try {
				image_view.image = app.image_editor.exec_operation.end (res);
			}
			catch (Error error) {
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

	Gtk.Builder builder;
	ulong width_changed_id = 0;
	ulong height_changed_id = 0;
	uint width;
	uint height;
	float ratio;
	ResizeUnit unit;
	Job preview_job;
	unowned Gtk.Adjustment zoom_adjustment;
	ulong zoom_adj_changed_id;
	bool high_quality = true;

	const double MAX_SIZE = 10000;
	const double MAX_PERCENT = 1000;
	const uint UPDATE_DELAY = 100;
	const float MIN_ZOOM = 0.05f;
	const float MAX_ZOOM = 3f;
	const float DEFAULT_MAX_SCALE_VALUE = 300;
}

public class Gth.ResizeOperation : ImageOperation {
	public ResizeOperation (uint _width, uint _height, bool _high_quality) {
		width = _width;
		height = _height;
		high_quality = _high_quality;
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable) {
		if (input == null) {
			return null;
		}
		return input.resize_to (width, height, high_quality ? ScaleFilter.BEST : ScaleFilter.POINT, cancellable);
	}

	uint width;
	uint height;
	bool high_quality;
}
