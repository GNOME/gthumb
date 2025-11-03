public class Gth.ColorPicker : ImageTool {
	public override void activate (Window _window) {
		window = _window;
		viewer = window.viewer.current_viewer as ImageViewer;
		init_actions ();

		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/color-picker.ui");
		window.editor.sidebar.child = builder.get_object ("options") as Gtk.Widget;
		window.editor.sidebar_header.title = _("Color Picker");
		window.editor.apply_button.visible = false;
		window.editor.content.child = builder.get_object ("main_view") as Gtk.Widget;
		window.editor.content.add_css_class ("image-view");

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.default_zoom_type = ZoomType.KEEP_PREVIOUS;
		image_view.image = viewer.image_view.image;
		image_view.zoom = viewer.image_view.zoom;
		image_view.hadjustment.value = viewer.image_view.hadjustment.value;
		image_view.vadjustment.value = viewer.image_view.vadjustment.value;

		position_x = builder.get_object ("position_x") as Adw.SpinRow;
		position_x.adjustment.configure (0, 1, image_view.image.get_width (), 10, 1, 0);
		position_x.adjustment.value_changed.connect (() => {
			show_color_at ((uint) position_x.adjustment.value, (uint) position_y.adjustment.value);
		});

		position_y = builder.get_object ("position_y") as Adw.SpinRow;
		position_y.adjustment.configure (0, 1, image_view.image.get_height (), 10, 1, 0);
		position_y.adjustment.value_changed.connect (() => {
			show_color_at ((uint) position_x.adjustment.value, (uint) position_y.adjustment.value);
		});

		hex_format = builder.get_object ("hex_format") as Adw.ActionRow;
		rgb_format = builder.get_object ("rgb_format") as Adw.ActionRow;
		rgb_percent_format = builder.get_object ("rgb_percent_format") as Adw.ActionRow;
		hsl_format = builder.get_object ("hsl_format") as Adw.ActionRow;
		color_preview = builder.get_object ("color_preview") as Gth.ColorPreview;

		// var motion_events = new Gtk.EventControllerMotion ();
		// motion_events.motion.connect ((x, y) => {
		// 	uint pixel_x, pixel_y;
		// 	image_view.get_pixel_at_position (x, y, out pixel_x, out pixel_y);
		// 	//stdout.printf ("> %u, %u\n", pixel_x, pixel_y);
		// 	position_x.adjustment.value = pixel_x + 1;
		// 	position_y.adjustment.value = pixel_y + 1;
		// });
		// image_view.add_controller (motion_events);

		var click_events = new Gtk.GestureClick ();
		click_events.set_button (Gdk.BUTTON_PRIMARY);
		var click_id = click_events.pressed.connect ((n_press, x, y) => {
			uint pixel_x, pixel_y;
			if (image_view.get_pixel_at_position (x, y, out pixel_x, out pixel_y)) {
				position_x.adjustment.value = pixel_x + 1;
				position_y.adjustment.value = pixel_y + 1;
				show_color_at (pixel_x, pixel_y);
			}
		});
		image_view.add_controller (click_events);
		image_view.cursor = new Gdk.Cursor.from_name ("crosshair", null);

		show_color_at (1, 1);
	}

	public override void deactivate () {
		builder = null;
		window.editor.content.remove_css_class ("image-view");
		window.insert_action_group ("tool", null);
	}

	void show_color_at (uint x, uint y) {
		if (image_view.image == null) {
			return;
		}
		uint8 red, green, blue, alpha;
		if (!image_view.image.get_rgba (x, y, out red, out green, out blue, out alpha)) {
			return;
		}
		//stdout.printf ("> PIXEL: rgb(%u,%u,%u), alpha: %u\n", red, green, blue, alpha);
		Gdk.RGBA color = {
			(float) red / 255,
			(float) green / 255,
			(float) blue / 255,
			(float) alpha / 255,
		};

		double h, s, l;
		rgb_to_hsl (red, green, blue, out h, out s, out l);
		if (h < 0) {
			h = 255 + h;
		}
		var h_int = (uint) Math.round (h / 255.0 * 360.0);
		var s_int = (uint) Math.round (s / 255.0 * 100.0);
		var l_int = (uint) Math.round (l / 255.0 * 100.0);

		var red_percent = (uint) Math.round (color.red * 100.0);
		var green_percent = (uint) Math.round (color.green * 100.0);
		var blue_percent = (uint) Math.round (color.blue * 100.0);

		if (alpha == 255) {
			hex_format.title = "#%02x%02x%02x".printf (red, green, blue);
			rgb_format.title = "rgb(%u, %u, %u)".printf (red, green, blue);
			rgb_percent_format.title = "rgb(%u%%, %u%%, %u%%)".printf (red_percent, green_percent, blue_percent);
			hsl_format.title = "hsl(%u, %u%%, %u%%)".printf (h_int, s_int, l_int);
		}
		else {
			var alpha_s = Util.format_double (color.alpha, "%.2f");
			hex_format.title = "#%02x%02x%02x%02x".printf (red, green, blue, alpha);
			rgb_format.title = "rgba(%u, %u, %u, %s)".printf (red, green, blue, alpha_s);
			rgb_percent_format.title = "rgba(%u%%, %u%%, %u%%, %s)".printf (red_percent, green_percent, blue_percent, alpha_s);
			hsl_format.title = "hsla(%u, %u%%, %u%%, %s)".printf (h_int, s_int, l_int, alpha_s);
		}

		color_preview.color = color;
	}

	void rgb_to_hsl (uint8 red, uint8 green, uint8 blue, out double hue, out double sat, out double lum) {
		var min = uint.min (uint.min (red, green), blue);
		var max = uint.max (uint.max (red, green), blue);

		lum = (max + min) / 2.0;
		if (max == min) {
			hue = sat = 0;
			return;
		}

		if (lum < 128) {
			sat = 255.0 * (max - min) / (max + min);
		}
		else {
			sat = 255.0 * (max - min) / (512.0 - max - min);
		}

		if (max == min) {
			hue = 0;
		}
		else if (max == red) {
			hue = 0.0 + 43.0 * (double) (green - blue) / (max - min);
		}
		else if (max == green) {
			hue = 85.0 + 43.0 * (double) (blue - red) / (max - min);
		}
		else if (max == blue) {
			hue = 171.0 + 43.0 * (double) (red - green) / (max - min);
		}
	}

	void init_actions () {
		action_group = new SimpleActionGroup ();
		window.insert_action_group ("tool", action_group);

		var action = new SimpleAction ("copy-color", VariantType.STRING);
		action.activate.connect ((_action, param) => {
			var name = param.get_string ();
			string value = null;
			if (name == "hex") {
				value = hex_format.title;
			}
			else if (name == "rgb") {
				value = rgb_format.title;
			}
			else if (name == "rgb_percent") {
				value = rgb_percent_format.title;
			}
			else if (name == "hsl") {
				value = hsl_format.title;
			}
			if (value != null) {
				window.copy_text_to_clipboard (value);
			}
		});
		action_group.add_action (action);
	}

	weak Window window;
	weak ImageViewer viewer;
	weak Gth.ImageView image_view;
	Gtk.Builder builder;
	SimpleActionGroup action_group;
	unowned Adw.SpinRow position_x;
	unowned Adw.SpinRow position_y;
	unowned Adw.ActionRow hex_format;
	unowned Adw.ActionRow rgb_format;
	unowned Adw.ActionRow rgb_percent_format;
	unowned Adw.ActionRow hsl_format;
	unowned Gth.ColorPreview color_preview;
}

public class Gth.ColorPreview : Gtk.Widget {
	public Gdk.RGBA color {
		set {
			_color = value;
			queue_draw ();
		}
		get {
			return _color;
		}
	}

	construct {
		add_css_class ("color-preview");
	}

	public override void measure (Gtk.Orientation orientation, int for_size, out int minimum, out int natural, out int minimum_baseline, out int natural_baseline) {
		if (orientation == Gtk.Orientation.HORIZONTAL) {
			minimum = NATURAL_WIDTH;
			natural = NATURAL_WIDTH;
			natural_baseline = -1;
			minimum_baseline = -1;
		}
		else {
			minimum = NATURAL_HEIGHT;
			natural = NATURAL_HEIGHT;
			natural_baseline = -1;
			minimum_baseline = -1;
		}
	}

	public override void size_allocate (int width, int height, int baseline) {
		viewport = {
			{ PADDING, PADDING },
			{ width - (PADDING * 2), height - (PADDING * 2) }
		};
	}

	public override void snapshot (Gtk.Snapshot snapshot) {
		var background = new Gsk.RoundedRect ();
		background.init_from_rect (viewport, BORDER_RADIUS);
		snapshot.push_rounded_clip (background);

		if (_color.alpha < 1f) {
			var ctx = snapshot.append_cairo (viewport);
			ctx.rectangle (viewport.origin.x, viewport.origin.y,
				viewport.size.width, viewport.size.height);
			var pattern = get_transparency_pattern ();
			var matrix = Cairo.Matrix.identity ();
			matrix.translate (- viewport.origin.x, - viewport.origin.y);
			pattern.set_matrix (matrix);
			ctx.set_source (pattern);
			ctx.fill ();
		}
		snapshot.append_color (_color, viewport);

		// Border
		Gdk.RGBA border_color = { 1.0f, 1.0f, 1.0f, 0.05f };
		float border_width = 2;
		snapshot.append_border (background,
			{ border_width, border_width, border_width, border_width } ,
			{ border_color, border_color, border_color, border_color }
		);

		snapshot.pop ();
	}

	Cairo.Pattern get_transparency_pattern () {
		if (transparency_pattern == null) {
			transparency_pattern = build_transparency_pattern (TRANSP_PATTERN_SIZE);
		}
		return transparency_pattern;
	}

	public static Cairo.Pattern build_transparency_pattern (int half_size) {
		var size = half_size * 2;
		var surface = new Cairo.ImageSurface (Cairo.Format.RGB24, size, size);
		var ctx = new Cairo.Context (surface);

		ctx.set_source_rgba (0.66, 0.66, 0.66, 1.0);
		ctx.rectangle (0, 0, half_size, half_size);
		ctx.fill ();
		ctx.rectangle (half_size, half_size, half_size, half_size);
		ctx.fill ();

		ctx.set_source_rgba (0.33, 0.33, 0.33, 1.0);
		ctx.rectangle (half_size, 0, half_size, half_size);
		ctx.fill ();
		ctx.rectangle (0, half_size, half_size, half_size);
		ctx.fill ();

		var transparency_pattern = new Cairo.Pattern.for_surface (surface);
		transparency_pattern.set_extend (Cairo.Extend.REPEAT);
		return transparency_pattern;
	}

	Graphene.Rect viewport;
	Gdk.RGBA _color;
	static Cairo.Pattern transparency_pattern = null;

	const int TRANSP_PATTERN_SIZE = 14; // pixels
	const int BORDER_RADIUS = 10;
	const int PADDING = 0;
	const int NATURAL_WIDTH = 70; // pixels
	const int NATURAL_HEIGHT = 50; // pixels
}
