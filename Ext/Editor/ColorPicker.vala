public class Gth.ColorPicker : ImageTool {
	public override void after_activate () {
		init_actions ();

		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/color-picker.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);
		window.editor.hide_apply ();

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		add_default_controllers (image_view);
		image_view.image = viewer.image_view.image;
		image_view.set_first_state_from_view (viewer.image_view);

		window.editor.set_content (image_view);

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((x, y) => {
			last_x = x;
			last_y = y;
		});
		image_view.add_controller (motion_events);

		var scroll_events = new Gtk.EventControllerScroll (Gtk.EventControllerScrollFlags.VERTICAL);
		scroll_events.scroll.connect ((controller, dx, dy) => {
			var step = (dy < 0) ? 0.1f : -0.1f;
			var new_zoom = image_view.zoom + (image_view.zoom * step);
			if ((last_x >= 0) && (last_y >= 0)) {
				image_view.set_zoom_and_center_at (new_zoom, (float) last_x, (float) last_y);
			}
			else {
				image_view.zoom = new_zoom;
			}
			return false;
		});
		image_view.add_controller (scroll_events);

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
		image_view.cursor = new Gdk.Cursor.from_name ("cell", null);

		show_color_at (1, 1);
	}

	public override void before_deactivate () {
		builder = null;
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

	construct {
		title = _("Color Picker");
		icon_name = "gth-eyedropper-symbolic";
	}

	Gtk.Builder builder;
	SimpleActionGroup action_group;
	unowned Adw.SpinRow position_x;
	unowned Adw.SpinRow position_y;
	unowned Adw.ActionRow hex_format;
	unowned Adw.ActionRow rgb_format;
	unowned Adw.ActionRow rgb_percent_format;
	unowned Adw.ActionRow hsl_format;
	unowned Gth.ColorPreview color_preview;
	double last_x = -1;
	double last_y = -1;
}
