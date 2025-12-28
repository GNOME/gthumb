public class Gth.ResizeImage : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/resize-image.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.image = original;
		add_default_controllers (image_view);
		window.editor.set_content (image_view);

		var spin_row = builder.get_object ("width") as Adw.SpinRow;
		width_changed_id = spin_row.adjustment.value_changed.connect ((obj) => {
			var local_adj = obj as Gtk.Adjustment;
			uint new_width, new_height;
			if (unit == Unit.PERCENT) {
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
			if (unit == Unit.PERCENT) {
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

		var button = new Adw.ButtonRow ();
		button.child = new_size_button_content (_("Reset"), original.width, original.height, "edit-undo-symbolic");
		button.activated.connect (() => reset_size ());
		special_size_group.add (button);

		/*var button = new Adw.ButtonRow ();
		button.child = new_size_button_content (_("100%"), original.width, original.height);
		button.activated.connect (() => reset_size ());
		special_size_group.add (button);

		button = new Adw.ButtonRow ();
		button.child = new_size_button_content (_("50%"), (uint) Math.round (original.width * 0.5), (uint) Math.round (original.height * 0.5));
		button.activated.connect (() => reset_size (0.5));
		special_size_group.add (button);

		button = new Adw.ButtonRow ();
		button.child = new_size_button_content (_("25%"), (uint) Math.round (original.width * 0.25), (uint) Math.round (original.height * 0.25));
		button.activated.connect (() => reset_size (0.25));
		special_size_group.add (button);*/

		var display = window.root.get_display ();
		var monitors = display.get_monitors ();
		var n_monitors = monitors.get_n_items ();
		for (var i = 0; i < n_monitors; i++) {
			var monitor = monitors.get_item (i) as Gdk.Monitor;
			var geometry = monitor.geometry;
			button = new Adw.ButtonRow ();
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

		var aspect_ratio_button = builder.get_object ("aspect_ratio_button") as Adw.ActionRow;
		aspect_ratio_button.activated.connect (() => {
			var page = builder.get_object ("aspect_ratio_page") as Adw.NavigationPage;
			window.editor.sidebar.push (page);
		});

		var aspect_ratio_group = builder.get_object ("aspect_ratio_group") as Gth.AspectRatioGroup;
		aspect_ratio_group.activate (window, original, true);
		aspect_ratio_group.changed.connect ((local_group, after_rotation) => {
			update_ratio (after_rotation);
			update_ratio_state ();
		});
		update_ratio_state ();

		var unit_group = builder.get_object ("unit_group") as Adw.ToggleGroup;
		unit_group.active_name = (unit == Unit.PIXEL) ? "pixels" : "percent";
		unit_group.notify["active-name"].connect ((obj, param) => {
			var local_group = obj as Adw.ToggleGroup;
			unit = (local_group.active_name == "pixels") ? Unit.PIXEL : Unit.PERCENT;
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
		return new ResizeOperation (width, height);
	}

	Gtk.Widget new_ratio_row (string name, uint idx, string? description = null) {
		var row = new Adw.ActionRow ();
		var check_button = new Gtk.CheckButton ();
		check_button.action_name = "resize.aspect-ratio";
		check_button.action_target = "%u".printf (idx);
		row.add_prefix (check_button);
		if (description != null) {
			var label = new Gtk.Label (description);
			label.add_css_class ("dimmed");
			row.add_suffix (label);
		}
		row.set_title (name);
		row.activatable = true;
		row.activatable_widget = check_button;
		return row;
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
		unit = Unit.PIXEL;
	}

	void set_size (uint _width, uint _height) {
		width = _width;
		height = _height;

		var spin_row = builder.get_object ("width") as Adw.SpinRow;
		SignalHandler.block (spin_row.adjustment, width_changed_id);
		spin_row.adjustment.freeze_notify ();
		if (unit == Unit.PERCENT) {
			spin_row.adjustment.lower = 1;
			spin_row.adjustment.upper = MAX_PERCENT;
			spin_row.adjustment.step_increment = 1;
			spin_row.adjustment.value = (double) width / original.width * 100;
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
		if (unit == Unit.PERCENT) {
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

	void update_ratio_state () {
		var aspect_ratio_group = builder.get_object ("aspect_ratio_group") as Gth.AspectRatioGroup;
		var aspect_ratio_button = builder.get_object ("aspect_ratio_button") as Adw.ActionRow;
		aspect_ratio_button.title = aspect_ratio_group.description;
	}

	Gtk.Builder builder;
	ulong width_changed_id = 0;
	ulong height_changed_id = 0;
	uint width;
	uint height;
	float ratio;
	Unit unit;
	Job preview_job;

	enum Unit {
		PIXEL,
		PERCENT
	}

	const double MAX_SIZE = 10000;
	const double MAX_PERCENT = 1000;
	const uint UPDATE_DELAY = 100;
}

public class Gth.ResizeOperation : ImageOperation {
	public ResizeOperation (uint _width, uint _height) {
		width = _width;
		height = _height;
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable) {
		if (input == null) {
			return null;
		}
		return input.resize_to (width, height, ScaleFilter.BEST, cancellable);
	}

	uint width;
	uint height;
}
