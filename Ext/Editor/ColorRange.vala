class Gth.ColorRange : Gtk.Widget {
	public ColorRangeType range_type {
		get { return _range_type; }
		construct { _range_type = value; }
	}

	public Gtk.Adjustment adjustment {
		get { return _adjustment; }
		set { configure_adjustment (value); }
	}

	public ColorRange (ColorRangeType _type, Gdk.RGBA _selected_color, Gdk.RGBA? _background_color = null) {
		Object (range_type: _type);
		background_color = _background_color;
		set_color (_selected_color);
	}

	public Gdk.RGBA get_color () {
		return selected_color;
	}

	public void set_color (Gdk.RGBA color) {
		selected_color = color;
		base_saturation = Color.get_saturation (selected_color);
		base_brightness = Color.get_brightness (selected_color);
		switch (_range_type) {
		case ColorRangeType.HUE:
			if (colors == null)
				build_hue_range ();
			var hue = Color.get_hue (selected_color);
			base_color = Color.get_rgba_from_hsv (hue, 1.0f, 1.0f);
			range_value = hue / 360;
			break;
		case ColorRangeType.SATURATION:
			if (base_saturation == 0)
				base_color = { 1f, 0, 0, 1f }; // Red
			else
				base_color = Color.change_saturation (selected_color, Color.get_max_saturation (selected_color));
			range_value = base_saturation;
			build_saturation_range ();
			break;
		case ColorRangeType.BRIGHTNESS:
			if (Color.is_black (selected_color))
				base_color = { 1f, 1f, 1f, 1f }; // White
			else
				base_color = Color.change_brightness (selected_color, Color.get_max_value (selected_color));
			range_value = base_brightness;
			build_brightness_range ();
			break;
		case ColorRangeType.RED:
			base_color = Color.change_component (selected_color, Color.Component.ALPHA, 1);
			range_value = selected_color.red;
			build_color_component_range (Color.Component.RED);
			break;
		case ColorRangeType.GREEN:
			base_color = Color.change_component (selected_color, Color.Component.ALPHA, 1);
			range_value = selected_color.green;
			build_color_component_range (Color.Component.GREEN);
			break;
		case ColorRangeType.BLUE:
			base_color = Color.change_component (selected_color, Color.Component.ALPHA, 1);
			range_value = selected_color.blue;
			build_color_component_range (Color.Component.BLUE);
			break;
		case ColorRangeType.ALPHA:
			base_color = Color.change_component (selected_color, Color.Component.ALPHA, 1);
			range_value = selected_color.alpha;
			build_color_component_range (Color.Component.ALPHA);
			break;
		case ColorRangeType.COLOR, ColorRangeType.PREVIEW:
			base_color = selected_color;
			range_value = 0;
			colors = null;
			break;
		case ColorRangeType.CYAN_RED:
			base_color = { 0, 0, 0, 1 };
			range_value = 0.5f;
			colors = {
				{ 0, { 0, 1, 1, 1 } },
				{ 1, { 1, 0, 0, 1 } },
			};
			break;
		case ColorRangeType.MAGENTA_GREEN:
			base_color = { 0, 0, 0, 1 };
			range_value = 0.5f;
			colors = {
				{ 0, { 1, 0, 1, 1 } },
				{ 1, { 0, 1, 0, 1 } },
			};
			break;
		case ColorRangeType.YELLOW_BLUE:
			base_color = { 0, 0, 0, 1 };
			range_value = 0.5f;
			colors = {
				{ 0, { 1, 1, 0, 1 } },
				{ 1, { 0, 0, 1, 1 } },
			};
			break;
		case ColorRangeType.BLACK_WHITE:
			base_color = { 0, 0, 0, 1 };
			range_value = 0.5f;
			colors = {
				{ 0, { 0, 0, 0, 1 } },
				{ 1, { 1, 1, 1, 1 } },
			};
			break;
		case ColorRangeType.WHITE_BLACK:
			base_color = { 0, 0, 0, 1 };
			range_value = 0.5f;
			colors = {
				{ 0, { 1, 1, 1, 1 } },
				{ 1, { 0, 0, 0, 1 } },
			};
			break;
		case ColorRangeType.GRAY_WHITE:
			base_color = { 0, 0, 0, 1 };
			range_value = 0.5f;
			colors = {
				{ 0, { 0.5f, 0.5f, 0.5f, 1 } },
				{ 1, { 1, 1, 1, 1 } },
			};
			break;
		}
		update_adj_value ();
		queue_draw ();
	}

	Gsk.ColorStop[] colors;
	Gtk.CssProvider css_provider;

	const string COMMON_STYLE = """
	.color-selector {
		outline: 0 solid transparent;
		outline-offset: 2px;
		transition-duration: 0.2s;
		transition-property: outline-color, outline-width, outline-offset;
		border-radius: 6px;
	}
	.color-selector.focusable:hover {
		outline: 2px solid alpha(@theme_fg_color, 0.3);
		outline-offset: 0px;
	}
	.color-selector.focusable:focus {
		outline: 2px solid;
		outline-offset: 0px;
	}
	.color-selector.focusable:active {
		outline: 2px solid;
		outline-offset: -1px;
	}
	""";

	const string DARK_STYLE = COMMON_STYLE + """
	.color-selector.focusable:focus {
		outline-color: alpha(var(--accent-color, #fff), 0.5);
	}
	.color-selector.focusable:active {
		outline-color: alpha(var(--accent-color, #fff), 0.7);
	}
	""";

	const string LIGHT_STYLE = COMMON_STYLE + """
	.color-selector.focusable:focus {
		outline-color: alpha(var(--accent-color, @theme_selected_bg_color), 0.5);
	}
	.color-selector.focusable:active {
		outline-color: alpha(var(--accent-color, @theme_selected_bg_color), 0.7);
	}
	""";

	public override void map () {
		css_provider = new Gtk.CssProvider ();
		Gtk.StyleContext.add_provider_for_display (Gdk.Display.get_default (), css_provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION);
		css_provider.load_from_string (theme_is_dark () ? DARK_STYLE : LIGHT_STYLE);
		base.map ();
	}

	public override void unmap () {
		css_provider = null;
		base.unmap ();
	}

	const int NATURAL_WIDTH = 70; // pixels
	// TODO: use the font height?
	const int NATURAL_HEIGHT = 36; // pixels

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

	const int H_PADDING = 4;
	const int V_PADDING = 4;

	public override void size_allocate (int width, int height, int baseline) {
		viewport = {
			{ H_PADDING, V_PADDING },
			{ width - (H_PADDING * 2), height - (V_PADDING * 2) }
		};
	}

	const float TRIANGLE_WIDTH = 17; // pixels
	const float TRIANGLE_HEIGHT = 10; // pixels
	const int BORDER_RADIUS = 4;

	public override void snapshot (Gtk.Snapshot snapshot) {
		var background = new Gsk.RoundedRect ();
		background.init_from_rect (viewport, BORDER_RADIUS);
		snapshot.push_rounded_clip (background);

		Gdk.RGBA border_color;
		float border_width;
		if (theme_is_dark ()) {
			border_color = { 1, 1, 1, 1 };
			border_width = 2;
		}
		else {
			border_color = { 0, 0, 0, 0.2f };
			border_width = 2;
		}

		var inner_rect = Graphene.Rect () {
			origin = { viewport.origin.x + border_width, viewport.origin.y + border_width },
			size = { viewport.size.width - (border_width * 2), viewport.size.height - (border_width * 2) },
		};

		// Trasparency pattern

		if (((_range_type == ColorRangeType.COLOR) || (_range_type == ColorRangeType.PREVIEW)) && (selected_color.alpha < 1f)
			|| (_range_type == ColorRangeType.ALPHA))
		{
			if (background_color != null) {
				snapshot.append_color (background_color, viewport);
			}
			else {
				var ctx = snapshot.append_cairo (viewport);
				ctx.rectangle (inner_rect.origin.x, inner_rect.origin.y, inner_rect.size.width, inner_rect.size.height);
				var pattern = get_transparency_pattern ();
				var matrix = Cairo.Matrix.identity ();
				matrix.translate (- inner_rect.origin.x, - inner_rect.origin.y);
				pattern.set_matrix (matrix);
				ctx.set_source (pattern);
				ctx.fill ();
			}
		}

		// Background

		switch (_range_type) {
		case ColorRangeType.COLOR, ColorRangeType.PREVIEW:
			snapshot.append_color (selected_color, viewport);
			break;

		default:
			var start = Graphene.Point () {
				x = 0,
				y = 0
			};
			var end = Graphene.Point () {
				x = viewport.size.width,
				y = 0
			};
			snapshot.append_linear_gradient (viewport, start, end, colors);
			break;
		}

		// Value

		if ((_range_type != ColorRangeType.COLOR) && (_range_type != ColorRangeType.PREVIEW)) {
			var inner_width = viewport.size.width - (border_width * 2);
			var value_x = viewport.origin.x + border_width + (inner_width * range_value);

			// Black downward-pointing triangle at the top
			var path = new Gsk.PathBuilder ();
			var triangle_width = TRIANGLE_WIDTH + 1;
			var triangle_height = TRIANGLE_HEIGHT + 1;
			path.move_to (value_x, viewport.origin.y + border_width + triangle_height);
			path.rel_line_to (-triangle_width / 2.0f, -triangle_height);
			path.rel_line_to (triangle_width, 0);
			path.close ();
			snapshot.append_fill (path.to_path (), Gsk.FillRule.EVEN_ODD, { 0, 0, 0, 1 });

			// White upward-pointing triangle at the bottom
			triangle_width = TRIANGLE_WIDTH - 1;
			triangle_height = TRIANGLE_HEIGHT - 1;
			path.move_to (value_x, viewport.origin.y + viewport.size.height - border_width - triangle_height);
			path.rel_line_to (-triangle_width / 2.0f, triangle_height);
			path.rel_line_to (triangle_width, 0);
			path.close ();
			snapshot.append_fill (path.to_path (), Gsk.FillRule.EVEN_ODD, { 1, 1, 1, 1 });
		}

		snapshot.pop ();

		// Border

		snapshot.append_border (background,
			{ border_width, border_width, border_width, border_width },
			{ border_color, border_color, border_color, border_color }
		);
	}

	const int TRANSP_PATTERN_SIZE = 14; // pixels
	static Cairo.Pattern transparency_pattern = null;

	Cairo.Pattern get_transparency_pattern () {
		if (transparency_pattern == null) {
			transparency_pattern = Color.build_transparency_pattern (TRANSP_PATTERN_SIZE);
		}
		return transparency_pattern;
	}

	void calc_color_from_adj_value () {
		var new_value = (float) ((_adjustment.value - _adjustment.lower) / (_adjustment.upper - _adjustment.lower));
		switch (_range_type) {
		case ColorRangeType.HUE:
			var hue = (float) _adjustment.value;
			base_color = Color.get_rgba_from_hsv (hue, 1.0f, 1.0f);
			selected_color = Color.get_rgba_from_hsv (hue, base_saturation, base_brightness);
			break;
		case ColorRangeType.SATURATION:
			selected_color = Color.change_saturation (base_color, new_value);
			break;
		case ColorRangeType.BRIGHTNESS:
			selected_color = Color.change_brightness (base_color, new_value);
			break;
		case ColorRangeType.RED:
			selected_color = Color.change_component (base_color, Color.Component.RED, new_value);
			break;
		case ColorRangeType.GREEN:
			selected_color = Color.change_component (base_color, Color.Component.GREEN, new_value);
			break;
		case ColorRangeType.BLUE:
			selected_color = Color.change_component (base_color, Color.Component.BLUE, new_value);
			break;
		case ColorRangeType.ALPHA:
			selected_color = Color.change_component (base_color, Color.Component.ALPHA, new_value);
			break;
		default:
			break;
		}
		range_value = new_value;
		queue_draw ();
		changed ();
	}

	void build_hue_range () {
		var step = 1f / 6;
		colors = {
			{ step * 0, Color.get_rgba (1, 0, 0) }, // RED
			{ step * 1, Color.get_rgba (1, 1, 0) }, // YELLOW
			{ step * 2, Color.get_rgba (0, 1, 0) }, // GREEN
			{ step * 3, Color.get_rgba (0, 1, 1) }, // CYAN
			{ step * 4, Color.get_rgba (0, 0, 1) }, // BLUE
			{ step * 5, Color.get_rgba (1, 0, 1) }, // MAGENTA
			{ step * 6, Color.get_rgba (1, 0, 0) }, // RED
		};
	}

	void build_saturation_range () {
		var max = Color.get_max_value (base_color);
		var color0 = Color.change_saturation (base_color, 0, max);
		var color1 = Color.change_saturation (base_color, 1, max);
		colors = {
			{ 0, color0 },
			{ 1, color1 },
		};
	}

	void build_brightness_range () {
		var max = Color.get_max_value (base_color);
		var color0 = Color.change_brightness (base_color, 0, max);
		var color1 = Color.change_brightness (base_color, 1, max);
		colors = {
			{ 0, color0 },
			{ 1, color1 },
		};
	}

	void build_color_component_range (Color.Component component) {
		var color0 = Color.change_component (base_color, component, 0);
		var color1 = Color.change_component (base_color, component, 1);
		colors = {
			{ 0, color0 },
			{ 1, color1 },
		};
	}

	public signal void changed ();

	void update_adj_value () {
		_adjustment.value = (range_value * (_adjustment.upper - _adjustment.lower)) + _adjustment.lower;
	}

	void on_click (double x, double y) {
		range_value = (float) ((x - viewport.origin.x) / viewport.size.width).clamp (0.0, 1.0);
		update_adj_value ();
		queue_draw ();
	}

	bool on_key_pressed (uint keyval, uint keycode, Gdk.ModifierType state) {
		var handled = false;
		switch (keyval) {
		case Gdk.Key.Right:
			_adjustment.value += (Gdk.ModifierType.SHIFT_MASK in state) ? _adjustment.page_increment : _adjustment.step_increment;
			handled = true;
			break;
		case Gdk.Key.Left:
			_adjustment.value -= (Gdk.ModifierType.SHIFT_MASK in state) ? _adjustment.page_increment : _adjustment.step_increment;
			handled = true;
			break;
		default:
			break;
		}
		return handled;
	}

	bool selecting = false;

	void on_button_pressed (int n_press, double x, double y) {
		if (n_press == 1) {
			grab_focus ();
			selecting = true;
			on_click (x, y);
		}
	}

	void on_button_released (int n_press, double x, double y) {
		selecting = false;
	}

	void on_motion (double x, double y) {
		if (selecting)
			on_click (x, y);
	}

	bool on_scroll (double dx, double dy) {
		if (dy < 0) {
			_adjustment.value -= _adjustment.step_increment;
		}
		else {
			_adjustment.value += _adjustment.step_increment;
		}
		return true;
	}

	bool theme_is_dark () {
		return true;
	}

	void configure_adjustment (Gtk.Adjustment? new_adjustment) {
		if (_adjustment == new_adjustment) {
			return;
		}
		if (adj_value_changed_id != 0) {
			_adjustment.disconnect (adj_value_changed_id);
			adj_value_changed_id = 0;
		}
		_adjustment = new_adjustment;
		if (_adjustment == null) {
			return;
		}
		switch (_range_type) {
		case ColorRangeType.HUE:
			_adjustment.configure (0.0, 0.0, 360.0, 1.0, 10.0, 0.0);
			break;
		case ColorRangeType.RED, ColorRangeType.GREEN, ColorRangeType.BLUE:
			_adjustment.configure (0.0, 0.0, 255.0, 1.0, 10.0, 0.0);
			break;
		case ColorRangeType.CYAN_RED, ColorRangeType.MAGENTA_GREEN,
			ColorRangeType.YELLOW_BLUE, ColorRangeType.BLACK_WHITE,
			ColorRangeType.WHITE_BLACK, ColorRangeType.GRAY_WHITE:
			// Configured by the widget user.
			break;
		default:
			_adjustment.configure (0.0, 0.0, 100.0, 1.0, 10.0, 0.0);
			break;
		}
		adj_value_changed_id = _adjustment.value_changed.connect (() => {
			calc_color_from_adj_value ();
		});
	}

	construct {
		css_provider = null;
		background_color = null;
		adjustment = new Gtk.Adjustment (0, 0, 0, 0, 0, 0);
		if (_range_type != ColorRangeType.PREVIEW) {
			focusable = true;
			add_css_class ("focusable");
			set_cursor_from_name ("pointer");

			var focus_events = new Gtk.EventControllerFocus ();
			focus_events.enter.connect (queue_draw);
			focus_events.leave.connect (queue_draw);
			add_controller (focus_events);

			var click_events = new Gtk.GestureClick ();
			click_events.pressed.connect (on_button_pressed);
			click_events.released.connect (on_button_released);
			add_controller (click_events);

			if (_range_type != ColorRangeType.COLOR) {
				var key_events = new Gtk.EventControllerKey ();
				key_events.key_pressed.connect (on_key_pressed);
				add_controller (key_events);

				var motion_events = new Gtk.EventControllerMotion ();
				motion_events.motion.connect (on_motion);
				add_controller (motion_events);

				var scroll_events = new Gtk.EventControllerScroll (Gtk.EventControllerScrollFlags.VERTICAL);
				scroll_events.scroll.connect (on_scroll);
				add_controller (scroll_events);
			}
		}
		add_css_class ("color-selector");
		//notify["display"].connect((s, p) => {
		//	var settings = get_settings ();
		//	css_provider.load_from_string (settings.gtk_application_prefer_dark_theme ? DARK_STYLE : LIGHT_STYLE);
		//});
		set_color (Color.get_rgba_from_hexcode ("#fff"));
		range_value = 0.5f;
	}

	Gdk.RGBA selected_color;
	Gdk.RGBA base_color;
	Gdk.RGBA? background_color;
	float base_saturation;
	float base_brightness;
	float range_value; // Always in [0,1]
	Graphene.Rect viewport;
	ColorRangeType _range_type;
	ulong adj_value_changed_id;
	Gtk.Adjustment _adjustment;
}
