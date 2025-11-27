public class Gth.ImageView : Gtk.Widget, Gtk.Scrollable {
	public Gth.Image image {
		get { return _image; }
		set {
			if (value == _image) {
				return;
			}
			before_changing_image ();
			if (_default_zoom_type == ZoomType.KEEP_PREVIOUS) {
				if (_image == null) {
					_zoom_type = ZoomType.MAXIMIZE_IF_LARGER;
				}
			}
			else {
				_zoom_type = _default_zoom_type;
			}
			_image = value;
			switch (_zoom_type) {
			case ZoomType.NATURAL_SIZE:
				set_zoom_and_update_scroll_offset (1f);
				break;
			case ZoomType.KEEP_PREVIOUS:
				set_zoom_and_update_scroll_offset (_zoom);
				break;
			default:
				break;
			}
			queue_resize ();
			start_animation ();
		}
	}

	public float zoom {
		get { return _zoom; }
		set {
			if ((_zoom_type == ZoomType.KEEP_PREVIOUS) && (value == _zoom)) {
				return;
			}
			_zoom_type = ZoomType.KEEP_PREVIOUS;
			if (_image != null) {
				set_zoom_and_keep_center_visible (value);
				queue_resize ();
			}
		}
	}

	public void set_zoom_and_center_at (float new_zoom, float x, float y) {
		if (_image != null) {
			_zoom_type = ZoomType.KEEP_PREVIOUS;
			_set_zoom_and_center_at (new_zoom, x, y);
			queue_resize ();
		}
	}

	public ZoomType zoom_type {
		get { return _zoom_type; }
		set {
			_zoom_type = value;
			if (_image != null) {
				if (_zoom_type == ZoomType.NATURAL_SIZE) {
					set_zoom_and_recenter_image (1f);
				}
				queue_resize ();
			}
		}
	}

	public ZoomType default_zoom_type {
		get { return _default_zoom_type; }
		set {
			_default_zoom_type = value;
		}
	}

	public TransparencyStyle transparency {
		get { return _transparency; }
		set {
			_transparency = value;
			queue_draw ();
		}
	}

	public Gtk.Adjustment hadjustment {
		get { return _hadjustment; }
		set construct {
			if ((_hadjustment != null) && (hadj_changed_id != 0)) {
				_hadjustment.disconnect (hadj_changed_id);
			}
			_hadjustment = value;
			if (_hadjustment != null) {
				update_scroll_offset ();
				hadj_changed_id = _hadjustment.value_changed.connect ((adj) => {
					viewport.origin = { (float) adj.get_value (), viewport.origin.y };
					update_texture_box ();
					update_image_box ();
					queue_draw ();
				});
			}
		}
	}

	public bool paused {
		get { return _paused; }
		set {
			_paused = value;
			if (_paused) {
				stop_animation ();
			}
			else if ((_image != null) && _image.get_is_animated ()) {
				start_animation ();
			}
		}
	}

	public Gtk.Adjustment vadjustment {
		get { return _vadjustment; }
		set construct {
			if ((_vadjustment != null) && (vadj_changed_id != 0)) {
				_vadjustment.disconnect (vadj_changed_id);
			}
			_vadjustment = value;
			if (_vadjustment != null) {
				update_scroll_offset ();
				vadj_changed_id = _vadjustment.value_changed.connect ((adj) => {
					viewport.origin = { viewport.origin.x, (float) adj.get_value () };
					update_texture_box ();
					update_image_box ();
					queue_draw ();
				});
			}
		}
	}

	public Gtk.ScrollablePolicy hscroll_policy { get; set; }

	public Gtk.ScrollablePolicy vscroll_policy { get; set; }

	public SimpleActionGroup action_group;

	public ImageController controller;

	public bool on_scroll (double dx, double dy, Gdk.ModifierType state) {
		var step = (dy < 0) ? 0.1f : -0.1f;
		var new_zoom = zoom + (zoom * step);
		if (last_position.is_valid ()) {
			set_zoom_and_center_at (new_zoom, last_position.x, last_position.y);
		}
		else {
			zoom = new_zoom;
		}
		return true;
	}

	public void add_scroll_controller () {
		var scroll_events = new Gtk.EventControllerScroll (Gtk.EventControllerScrollFlags.VERTICAL);
		scroll_events.scroll.connect ((controller, dx, dy) => {
			return on_scroll (dx, dy, controller.get_current_event_state ());
		});
		add_controller (scroll_events);
	}

	public void add_drag_controller () {
		var click_events = new Gtk.GestureClick ();
		click_events.button = Gdk.BUTTON_PRIMARY;
		click_events.pressed.connect ((n_press, x, y) => {
			//stdout.printf ("> PRESSED %f,%f\n", x, y);
			clicking = true;
			dragging = false;
			drag_start = ClickPoint (x, y);
			prev_cursor = cursor;
		});
		click_events.released.connect ((n_press, x, y) => {
			//stdout.printf ("> RELEASED\n");
			clicking = false;
			dragging = false;
			cursor = prev_cursor;
		});
		add_controller (click_events);

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((x, y) => {
			if (!dragging && clicking) {
				if (drag_start.drag_threshold (x, y)) {
					dragging = true;
					cursor = new Gdk.Cursor.from_name ("grabbing", null);
				}
			}
			if (dragging) {
				var dx = (float) (drag_start.x - x);
				var dy = (float) (drag_start.y - y);
				// stdout.printf ("scroll_by: %f,%f\n", dx, dy);
				scroll_by (dx, dy);
				drag_start = ClickPoint (x, y);
			}
			last_position = ClickPoint (x, y);
		});
		add_controller (motion_events);
	}

	public bool get_border (out Gtk.Border border)  {
		border = Gtk.Border();
		return false;
	}

	public signal void resized ();
	public signal void scrolled ();

	public override Gtk.SizeRequestMode get_request_mode () {
		return Gtk.SizeRequestMode.HEIGHT_FOR_WIDTH;
	}

	public override void measure (Gtk.Orientation orientation, int for_size, out int minimum, out int natural, out int minimum_baseline, out int natural_baseline) {
		// stdout.printf ("> ImageView.measure: orientation: %s, for_size: %d\n", orientation.to_string (), for_size);
		minimum = 0;
		natural = 0;
		natural_baseline = -1;
		minimum_baseline = -1;
		if ((for_size == -1) && (width_request == -1) && (height_request == -1)
			&& (halign == Gtk.Align.FILL) && (valign == Gtk.Align.FILL))
		{
			// Can adapt to any size.
			// stdout.printf ("  natural [0]: %d\n", natural);
			return;
		}
		if ((width_request != -1) && ((width_request < for_size) || (for_size == -1))) {
			// Respect width_request
			for_size = width_request;
		}
		if ((height_request != -1) && ((height_request < for_size) || (for_size == -1))) {
			// Respect height_request
			for_size = height_request;
		}
		if (_image != null) {
			uint natural_width, natural_height;
			_image.get_natural_size (out natural_width, out natural_height);
			if (_zoom_type == KEEP_PREVIOUS) {
				float zoomed_width, zoomed_height;
				get_zoomed_size_for_zoom (_zoom, out zoomed_width, out zoomed_height);
				natural_width = (uint) zoomed_width;
				natural_height = (uint) zoomed_height;
			}
			else if (_zoom_type != ZoomType.NATURAL_SIZE) {
				if (for_size > 0) {
					_image.get_natural_size (out natural_width, out natural_height);
					var new_zoom = Util.get_zoom_to_fit_surface (natural_width, natural_height, for_size, for_size);
					natural_width = (uint) (new_zoom * natural_width);
					natural_height = (uint) (new_zoom * natural_height);
				}
			}
			if (orientation == Gtk.Orientation.HORIZONTAL) {
				natural = (int) natural_width;
			}
			else {
				natural = (int) natural_height;
			}
		}
		// stdout.printf ("  natural [1]: %d\n", natural);
	}

	public override void realize () {
		base.realize ();
		if (controller != null) {
			controller.on_realize ();
		}
	}

	public override void unrealize () {
		base.unrealize ();
		if (controller != null) {
			controller.on_unrealize ();
		}
	}

	public override void size_allocate (int width, int height, int baseline) {
		viewport.size = { width, height };

		// stdout.printf ("> size_allocate: width: %d (%f), height: %d (%f), zoom: %f\n",
		// 	width, viewport.size.width,
		// 	height, viewport.size.height,
		// 	_zoom);

		var offset_updated = false;
		if (_image != null) {
			if (_zoom_type.fit_to_allocation ()) {
				set_valid_zoom (get_zoom_for_allocation (_zoom_type, width, height));
				recenter_image ();
			}
			else {
				if (_first_allocation) {
					recenter_image ();
				}
				else {
					update_scroll_offset ();
					offset_updated = true;
				}
			}
			_first_allocation = false;
		}
		update_texture_box ();
		update_image_box ();
		if (!offset_updated) {
			update_scroll_offset ();
		}
		if (controller != null) {
			controller.on_size_allocated ();
		}
		resized ();
	}

	public bool get_pixel_at_position (double pointer_x, double pointer_y, out uint pixel_x, out uint pixel_y) {
		var inside = true;
		var px = (pointer_x - texture_box.origin.x + viewport.origin.x) / _zoom;
		if (px < 0) {
			px = 0;
			inside = false;
		}
		pixel_x = (uint) px;
		var max_x = image.get_width () - 1;
		if (pixel_x > max_x) {
			pixel_x = max_x;
			inside = false;
		}

		var py = (pointer_y - texture_box.origin.y + viewport.origin.y) / _zoom;
		if (py < 0) {
			py = 0;
			inside = false;
		}
		pixel_y = (uint) py;
		var max_y = image.get_height () - 1;
		if (pixel_y > max_y) {
			pixel_y = max_y;
			inside = false;
		}
		return inside;
	}

	public override void snapshot (Gtk.Snapshot snapshot) {
		if (_image == null) {
			return;
		}
		// Util.print_rectangle ("> image_box:", image_box);
		// Util.print_rectangle ("> texture_box:", texture_box);
		snapshot.save ();

		bool has_alpha;
		if (image.get_has_alpha (out has_alpha) && has_alpha) {
			switch (_transparency) {
			case TransparencyStyle.WHITE, TransparencyStyle.BLACK:
				Gdk.RGBA rgba;
				_transparency.get_rgba (out rgba);
				snapshot.append_color (rgba, texture_box);
				break;

			case TransparencyStyle.CHECKERED:
				var ctx = snapshot.append_cairo (texture_box);
				ctx.rectangle (texture_box.origin.x, texture_box.origin.y,
					texture_box.size.width, texture_box.size.height);
				ctx.set_source (get_transparency_pattern ());
				ctx.fill ();
				break;

			case TransparencyStyle.GRAY:
				break;
			}
		}

		if (_image.get_is_scalable ()) {
			var texture = _image.get_scaled_texture (
				_zoom,
				(uint) image_box.origin.x,
				(uint) image_box.origin.y,
				(uint) texture_box.size.width,
				(uint) texture_box.size.height);
			if (texture != null) {
				var ctx = snapshot.append_cairo (texture_box);
				ctx.rectangle (texture_box.origin.x, texture_box.origin.y,
					texture_box.size.width, texture_box.size.height);
				ctx.set_source_surface (texture, texture_box.origin.x, texture_box.origin.y);
				ctx.fill ();
			}
		}
		else if (scaled_texture != null) {
			// stdout.printf ("> TEXTURE BOX: %f,%f\n", texture_box.size.width, texture_box.size.height);
			// stdout.printf ("  SCALED TEXTURE: %d,%d\n", scaled_texture.width, scaled_texture.height);
			snapshot.append_scaled_texture (scaled_texture, Gsk.ScalingFilter.NEAREST, texture_box);
		}
		else {
			var texture = _image.get_texture_for_rect (
				(uint) image_box.origin.x,
				(uint) image_box.origin.y,
				(uint) image_box.size.width,
				(uint) image_box.size.height,
				current_frame);
			if (texture != null) {
				snapshot.append_scaled_texture (texture, _zoom > 4 ? Gsk.ScalingFilter.NEAREST : Gsk.ScalingFilter.LINEAR, texture_box);
			}
		}

		if (controller != null) {
			snapshot.push_clip (texture_box);
			controller.on_snapshot (snapshot);
			snapshot.pop ();
		}

		if (selection_box.size.width > 0) {
			// var ctx = snapshot.append_cairo (selection_box);
			// ctx.rectangle (selection_box.origin.x, selection_box.origin.y,
			// 	selection_box.size.width, selection_box.size.height);
			// ctx.set_source_rgba (1, 0, 0, 0.75);
			// ctx.set_line_width (4);
			// ctx.stroke ();

			var box = selection_box;
			box.origin.x += texture_box.origin.x;
			box.origin.y += texture_box.origin.y;

			Gdk.RGBA background_color = { 0, 0, 0, 0.25f };
			var round_x =  Math.roundf (box.origin.x);
			snapshot.append_color (background_color, {
				{ 0, 0 },
				{ round_x, viewport.size.height }
			});
			var round_width = Math.roundf (box.size.width);
			snapshot.append_color (background_color, {
				{ round_x + round_width, 0 },
				{ viewport.size.width - (round_x + round_width) - 0.5f, viewport.size.height }
			});
			var round_y = Math.roundf (box.origin.y);
			snapshot.append_color (background_color, {
				{ round_x, 0 },
				{ round_width, round_y }
			});
			var round_height = Math.roundf (box.size.height);
			snapshot.append_color (background_color, {
				{ round_x, round_y + round_height },
				{ round_width, viewport.size.height - (round_y + round_height) }
			});

			var rect = new Gsk.RoundedRect ();
			rect.init_from_rect (box, 0);
			Gdk.RGBA border_color = { 1, 1, 1, 1 };
			var border_width = 2f;
			snapshot.append_border (rect,
				{ border_width, border_width, border_width, border_width },
				{ border_color, border_color, border_color, border_color }
			);
		}

		snapshot.restore ();
	}

	const int TRANSP_PATTERN_SIZE = 14; // pixels
	static Cairo.Pattern transparency_pattern = null;

	unowned Cairo.Pattern get_transparency_pattern () {
		if (transparency_pattern == null) {
			transparency_pattern = Util.build_transparency_pattern (TRANSP_PATTERN_SIZE);
		}
		return transparency_pattern;
	}

	public void scroll_to (double x, double y) {
		set_scroll_offset ((float) x, (float) y);
		update_texture_box ();
		update_image_box ();
		queue_draw ();
	}

	public void scroll_by (double dx, double dy) {
		set_scroll_offset (viewport.origin.x + (float) dx, viewport.origin.y + (float) dy);
		update_texture_box ();
		update_image_box ();
		queue_draw ();
	}

	public float get_zoom_for_type (ZoomType type) {
		return get_zoom_for_allocation (type, (int) viewport.size.width, (int) viewport.size.height);
	}

	float get_zoom_for_allocation (ZoomType type, int width, int height) {
		if (image == null) {
			return _zoom;
		}
		float new_zoom = _zoom;
		uint natural_width, natural_height;
		_image.get_natural_size (out natural_width, out natural_height);
		switch (type) {
		case ZoomType.MAXIMIZE:
			new_zoom = Util.get_zoom_to_fit_surface (natural_width, natural_height, width, height);
			/*stdout.printf ("[%u,%u] in [%d,%d] -> %f\n",
				natural_width, natural_height, width, height,
				new_zoom);*/
			break;

		case ZoomType.MAXIMIZE_IF_LARGER:
			if ((natural_width > width) || (natural_height > height)) {
				new_zoom = Util.get_zoom_to_fit_surface (natural_width, natural_height, width, height);
			}
			else {
				new_zoom = 1f;
			}
			break;

		case ZoomType.BEST_FIT:
			new_zoom = Util.get_zoom_to_fit_or_fill_surface (natural_width, natural_height, width, height);
			break;

		case ZoomType.MAXIMIZE_WIDTH:
			new_zoom = Util.get_zoom_to_fit_length (natural_width, width);
			break;

		case ZoomType.MAXIMIZE_HEIGHT:
			new_zoom = Util.get_zoom_to_fit_length (natural_height, height);
			break;

		case ZoomType.FILL_SPACE:
			new_zoom = Util.get_zoom_to_fill_surface (natural_width, natural_height, width, height);
			break;

		default:
			break;
		}
		return new_zoom;
	}

	public void recenter () {
		recenter_image ();
		queue_resize ();
	}

	void recenter_image () {
		float zoomed_width, zoomed_height;
		get_zoomed_size_for_zoom (_zoom, out zoomed_width, out zoomed_height);

		var offset_x = Util.center_content (zoomed_width, viewport.size.width);
		var offset_y = Util.center_content (zoomed_height, viewport.size.height);
		set_scroll_offset (offset_x, offset_y);
	}

	float set_valid_zoom (float new_zoom) {
		_zoom = new_zoom.clamp (MIN_ZOOM, MAX_ZOOM);
		return _zoom;
	}

	void set_zoom_and_recenter_image (float new_zoom) {
		set_valid_zoom (new_zoom);
		recenter_image ();
	}

	void set_zoom_and_update_scroll_offset (float new_zoom) {
		set_valid_zoom (new_zoom);
		update_scroll_offset ();
	}

	void set_zoom_and_keep_center_visible (float new_zoom) {
		_set_zoom_and_center_at (new_zoom, viewport.size.width / 2, viewport.size.height / 2);
	}

	void _set_zoom_and_center_at (float new_zoom, float pointer_x, float pointer_y) {
		var old_zoom = _zoom;
		new_zoom = set_valid_zoom (new_zoom);
		if ((texture_box.size.width == 0) || (viewport.size.width == 0)) {
			update_adjustments (0, 0);
		}
		else {
			// Pixel under the pointer.
			var x = (viewport.origin.x - texture_box.origin.x + pointer_x) / old_zoom;
			var y = (viewport.origin.y - texture_box.origin.y + pointer_y) / old_zoom;

			// Pixel at the center of the visibile area.
			var x0 = (viewport.origin.x - texture_box.origin.x + (viewport.size.width / 2)) / old_zoom;
			var y0 = (viewport.origin.y - texture_box.origin.y + (viewport.size.height / 2)) / old_zoom;

			// Delta to keep the (x, y) pixel under the pointer.
			var dx = ((x - x0) * new_zoom) - ((x - x0) * old_zoom);
			var dy = ((y - y0) * new_zoom) - ((y - y0) * old_zoom);

			// Center on (x0, y0) and add (dx, dy)
			var center_x = (x0 * new_zoom) + dx;
			var center_y = (y0 * new_zoom) + dy;

			// Offset to center the pointer.
			var offset_x = center_x - (viewport.size.width / 2);
			var offset_y = center_y - (viewport.size.height / 2);
			update_adjustments (offset_x, offset_y);
		}
	}

	void update_texture_box () {
		float zoomed_width, zoomed_height;
		get_zoomed_size_for_zoom (_zoom, out zoomed_width, out zoomed_height);

		float texture_x, texture_width;
		if (viewport.origin.x + zoomed_width > viewport.size.width) {
			texture_x = 0;
			texture_width = viewport.size.width;
		}
		else {
			texture_width = Math.roundf (zoomed_width);
			texture_x = Util.center_content (viewport.size.width, zoomed_width);
		}

		float texture_y, texture_height;
		if (viewport.origin.y + zoomed_height > viewport.size.height) {
			texture_y = 0;
			texture_height = viewport.size.height;
		}
		else {
			texture_height = Math.roundf (zoomed_height);
			texture_y = Util.center_content (viewport.size.height, zoomed_height);
		}
		texture_box = {
			{ texture_x, texture_y },
			{ texture_width, texture_height }
		};
		// Util.print_rectangle ("> update_texture_box:", texture_box);
	}

	// To be called after updating texture_box.
	void update_image_box () {
		invalidate_scaled_texture ();
		cancel_filter_update ();

		if (_image == null) {
			image_box = { { 0, 0 },	{ 0, 0 } };
			return;
		}

		uint natural_width, natural_height;
		_image.get_natural_size (out natural_width, out natural_height);

		float image_x, image_width;
		if (texture_box.origin.x > 0) {
			image_x = 0;
			image_width = natural_width;
		}
		else {
			image_x = viewport.origin.x / _zoom;
			image_width = (float) viewport.size.width / _zoom;
		}

		float image_y, image_height;
		if (texture_box.origin.y > 0) {
			image_y = 0;
			image_height = natural_height;
		}
		else {
			image_y = viewport.origin.y / _zoom;
			image_height = (float) viewport.size.height / _zoom;
		}

		image_box = {
			{ image_x, image_y },
			{ image_width, image_height }
		};
		queue_update_scaled_texture ();
		//Util.print_rectangle ("> update_image_box: ", image_box);
	}

	void get_zoomed_size_for_zoom (float zoom_level, out float width, out float height) {
		if (_image != null) {
			uint natural_width, natural_height;
			_image.get_natural_size (out natural_width, out natural_height);
			width = zoom_level * natural_width;
			height = zoom_level * natural_height;
		}
		else {
			width = 0f;
			height = 0f;
		}
	}

	void set_origin (float x, float y, float zoomed_width, float zoomed_height) {
		var max_x = float.max (zoomed_width - viewport.size.width, 0);
		var max_y = float.max (zoomed_height - viewport.size.height, 0);
		//stdout.printf ("> set_scroll_offset x: %f (max: %f), y: %f (max: %f) (zoom_level: %f)\n", x, max_x, y, max_y, zoom_level);
		x = x.clamp (0, max_x);
		y = y.clamp (0, max_y);
		viewport.origin = { x, y };
		scrolled ();
	}

	void set_scroll_offset (float x, float y) {
		float zoomed_width, zoomed_height;
		get_zoomed_size_for_zoom (_zoom, out zoomed_width, out zoomed_height);
		set_origin (x, y, zoomed_width, zoomed_height);
		if (_hadjustment != null) {
			if (hadj_changed_id != 0) {
				SignalHandler.block (_hadjustment, hadj_changed_id);
			}
			_hadjustment.set_value (x);
			if (hadj_changed_id != 0) {
				SignalHandler.unblock (_hadjustment, hadj_changed_id);
			}
		}
		if (_vadjustment != null) {
			if (vadj_changed_id != 0) {
				SignalHandler.block (_vadjustment, vadj_changed_id);
			}
			_vadjustment.set_value (y);
			if (vadj_changed_id != 0) {
				SignalHandler.unblock (_vadjustment, vadj_changed_id);
			}
		}
	}

	const double STEP_INCREMENT = 0.1;
	const double PAGE_INCREMENT = 0.5;

	void update_adjustments (float x, float y) {
		float zoomed_width, zoomed_height;
		get_zoomed_size_for_zoom (_zoom, out zoomed_width, out zoomed_height);
		set_origin (x, y, zoomed_width, zoomed_height);
		if (_vadjustment != null) {
			_vadjustment.freeze_notify ();
			_vadjustment.set_lower (0);
			if (zoomed_height - viewport.size.height >= 1) {
				_vadjustment.set_upper (zoomed_height);
				_vadjustment.set_page_size (viewport.size.height);
			}
			else {
				_vadjustment.set_upper (zoomed_height);
				_vadjustment.set_page_size (zoomed_height);
			}
			_vadjustment.set_step_increment (_vadjustment.page_size * STEP_INCREMENT);
			_vadjustment.set_page_increment (_vadjustment.page_size * PAGE_INCREMENT);
			_vadjustment.set_value (viewport.origin.y);
			_vadjustment.thaw_notify ();
		}
		if (_hadjustment != null) {
			_hadjustment.freeze_notify ();
			_hadjustment.set_lower (0);
			if (zoomed_width - viewport.size.width > 1) {
				_hadjustment.set_upper (zoomed_width);
				_hadjustment.set_page_size (viewport.size.width);
			}
			else {
				_hadjustment.set_upper (zoomed_width);
				_hadjustment.set_page_size (zoomed_width);
			}
			_hadjustment.set_step_increment (_hadjustment.page_size * STEP_INCREMENT);
			_hadjustment.set_page_increment (_hadjustment.page_size * PAGE_INCREMENT);
			_hadjustment.set_value (viewport.origin.x);
			_hadjustment.thaw_notify ();
		}
	}

	void update_scroll_offset () {
		// Make sure the scroll offsets are valid.
		update_adjustments (viewport.origin.x, viewport.origin.y);
	}

	public override bool focus (Gtk.DirectionType direction) {
		if (!is_focus ()) {
			grab_focus ();
			return true;
		}
		return false;
	}

	public override void map () {
		base.map ();
		start_animation ();
	}

	public override void unmap () {
		base.unmap ();
		stop_animation ();
	}

	public void next_frame () {
		if ((image != null) && image.next_frame (ref current_frame)) {
			queue_draw ();
		}
	}

	void stop_animation () {
		if (animation_id != 0) {
			Source.remove (animation_id);
			animation_id = 0;
		}
	}

	void start_animation () {
		if (!get_mapped () || _paused || (_image == null) || !_image.get_is_animated ()) {
			return;
		}
		if (animation_id == 0) {
			animation_id = Timeout.add (ANIMATION_DELAY, () => {
				current_time += ANIMATION_DELAY;
				var _continue = _image.get_frame_at (ref current_time, out current_frame);
				queue_draw ();
				return _continue ? Source.CONTINUE : Source.REMOVE;
			});
		}
	}

	void cancel_filter_update () {
		if (filter_id != 0) {
			Source.remove (filter_id);
			filter_id = 0;
		}
		if (filter_cancellable != null) {
			filter_cancellable.cancel ();
		}
	}

	//uint filter_count = 0;

	void queue_update_scaled_texture () {
		filter_id = Util.after_next_rearrange (() => {
			filter_id = 0;
			update_scaled_texture ();
		});
	}

	void update_scaled_texture () {
		if ((_image == null) || _image.get_is_scalable () || _image.get_is_animated ()) {
			return;
		}
		if (_zoom >= 1f) {
			return;
		}
		if ((texture_box.size.width == 0) || (texture_box.size.height == 0)) {
			return;
		}
		if ((image_box.size.width == 0) || (image_box.size.height == 0)) {
			return;
		}

		var scaler = new TextureScaler ();
		scaler.image_box = image_box;
		scaler.texture_box = texture_box;
		scaler.filter = (image_box.size.width * image_box.size.height >= BIG_IMAGE) ? ScaleFilter.BOX : ScaleFilter.TRIANGLE;

		// filter_count++;
		// stdout.printf ("> UPDATE SCALED TEXTURE %u\n", filter_count);
		// stdout.printf ("  IMAGE BOX SIZE: %f, %f\n", scaler.image_box.size.width, scaler.image_box.size.height);
		// stdout.printf ("  TEXTURE BOX SIZE: %f, %f\n", scaler.texture_box.size.width, scaler.texture_box.size.height);
		// stdout.printf ("  FILTER: %d\n", scaler.filter);

		var local_cancellable = new Cancellable ();
		filter_cancellable = local_cancellable;
		app.image_editor.exec_operation.begin (_image, scaler, local_cancellable, (obj, res) => {
			try {
				var scaled = app.image_editor.exec_operation.end (res);
				scaled_texture = scaled.get_texture ();
				// stdout.printf ("< SCALED TEXTURE READY!\n");
				queue_draw ();
			}
			catch (Error error) {
				// stdout.printf ("! SCALED TEXTURE CANCELLED\n");
			}
			if (filter_cancellable == local_cancellable) {
				filter_cancellable = null;
			}
		});
	}

	void before_changing_image () {
		stop_animation ();
		cancel_filter_update ();
		viewport.size = { 0, 0 };
		image_box = { { 0, 0 }, { 0, 0 } };
		texture_box = { { 0, 0 }, { 0, 0 } };
		scaled_texture = null;
	}

	void invalidate_scaled_texture () {
		scaled_texture = null;
	}

	public void set_selection (Graphene.Rect selection) {
		selection_box = selection;
		queue_draw ();
	}

	public void remove_selection () {
		selection_box = { { 0, 0 }, { 0, 0 } };
		queue_draw ();
	}

	void init_actions () {
		action_group = new SimpleActionGroup ();

		var action = new SimpleAction.stateful ("zoom-100", null, new Variant.boolean (false));
		action.activate.connect ((action, param) => {
			if (zoom_type != ZoomType.NATURAL_SIZE) {
				zoom_type = ZoomType.NATURAL_SIZE;
			}
			else {
				zoom_type = ZoomType.BEST_FIT;
			}
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("zoom-best-fit", null, new Variant.boolean (false));
		action.activate.connect ((_action, param) => {
			if (zoom_type != ZoomType.BEST_FIT) {
				zoom_type = ZoomType.BEST_FIT;
			}
			else {
				zoom_type = ZoomType.MAXIMIZE_IF_LARGER;
			}
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("zoom-view-all", null, new Variant.boolean (false));
		action.activate.connect ((_action, param) => {
			if (zoom_type != ZoomType.MAXIMIZE_IF_LARGER) {
				zoom_type = ZoomType.MAXIMIZE_IF_LARGER;
			}
			else {
				zoom_type = ZoomType.BEST_FIT;
			}
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("set-zoom", VariantType.STRING, new Variant.string (""));
		action.activate.connect ((action, param) => {
			unowned var value = param.get_string ();
			action.set_state (new Variant.string (value));
			switch (value) {
			case "best-fit":
				zoom_type = ZoomType.BEST_FIT;
				break;
			case "max-size":
				zoom_type = ZoomType.MAXIMIZE;
				break;
			case "max-size-if-larger":
				zoom_type = ZoomType.MAXIMIZE_IF_LARGER;
				break;
			case "max-width":
				zoom_type = ZoomType.MAXIMIZE_WIDTH;
				break;
			case "max-height":
				zoom_type = ZoomType.MAXIMIZE_HEIGHT;
				break;
			case "fill-space":
				zoom_type = ZoomType.FILL_SPACE;
				break;
			case "50":
				zoom = 0.5f;
				break;
			case "100":
				zoom = 1f;
				break;
			case "200":
				zoom = 2f;
				break;
			case "300":
				zoom = 3f;
				break;
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("zoom-in", null);
		action.activate.connect ((_action, param) => {
			zoom += _zoom * 0.1f;
		});
		action_group.add_action (action);

		action = new SimpleAction ("zoom-out", null);
		action.activate.connect ((_action, param) => {
			zoom -= _zoom * 0.1f;
		});
		action_group.add_action (action);

		action = new SimpleAction ("zoom-in-fast", null);
		action.activate.connect ((_action, param) => {
			zoom += _zoom * 0.5f;
		});
		action_group.add_action (action);

		action = new SimpleAction ("zoom-out-fast", null);
		action.activate.connect ((_action, param) => {
			zoom -= _zoom * 0.5f;
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("set-transparency", VariantType.STRING, new Variant.string (transparency.get_state ()));
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
			transparency = style;
		});
		action_group.add_action (action);

		action = new SimpleAction ("scroll-left", null);
		action.activate.connect (() => {
			scroll_by (0 - hadjustment.step_increment, 0);
		});
		action_group.add_action (action);

		action = new SimpleAction ("scroll-right", null);
		action.activate.connect (() => {
			scroll_by (hadjustment.step_increment, 0);
		});
		action_group.add_action (action);

		action = new SimpleAction ("scroll-up", null);
		action.activate.connect (() => {
			scroll_by (0, 0 - vadjustment.step_increment);
		});
		action_group.add_action (action);

		action = new SimpleAction ("scroll-down", null);
		action.activate.connect (() => {
			scroll_by (0, vadjustment.step_increment);
		});
		action_group.add_action (action);

		action = new SimpleAction ("scroll-page-left", null);
		action.activate.connect (() => {
			scroll_by (-hadjustment.page_increment, 0);
		});
		action_group.add_action (action);

		action = new SimpleAction ("scroll-page-right", null);
		action.activate.connect (() => {
			scroll_by (hadjustment.page_increment, 0);
		});
		action_group.add_action (action);

		action = new SimpleAction ("scroll-page-up", null);
		action.activate.connect (() => {
			scroll_by (0, -vadjustment.page_increment);
		});
		action_group.add_action (action);

		action = new SimpleAction ("scroll-page-down", null);
		action.activate.connect (() => {
			scroll_by (0, vadjustment.page_increment);
		});
		action_group.add_action (action);

		action = new SimpleAction ("recenter", null);
		action.activate.connect (() => {
			recenter ();
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("toggle-paused", null, new Variant.boolean (false));
		action.activate.connect ((action, param) => {
			if (image != null) {
				paused = Util.toggle_state (action);
				Util.enable_action (action_group, "next-frame", paused);
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("next-frame", null);
		action.activate.connect ((action, param) => {
			next_frame ();
		});
		action.set_enabled (false);
		action_group.add_action (action);
	}

	construct {
		_image = null;
		_zoom_type = ZoomType.BEST_FIT;
		_default_zoom_type = ZoomType.KEEP_PREVIOUS;
		_transparency = TransparencyStyle.GRAY;
		_zoom = 1f;
		_first_allocation = true;
		hadj_changed_id = 0;
		vadj_changed_id = 0;
		focusable = true;
		animation_id = 0;
		current_time = 0;
		current_frame = 0;
		filter_id = 0;
		_paused = false;
		scaled_texture = null;
		filter_cancellable = null;
		selection_box = { { 0, 0 }, { 0, 0 } };
		dragging = false;
		clicking = false;
		drag_start = { -1, -1 };
		last_position = { -1, -1 };
		init_actions ();
		hadjustment = new Gtk.Adjustment (0, 0, 0, 0, 0, 0);
		vadjustment = new Gtk.Adjustment (0, 0, 0, 0, 0, 0);
		controller = null;
	}

	Gth.Image _image;
	Gth.ZoomType _zoom_type;
	Gth.ZoomType _default_zoom_type;
	Gth.TransparencyStyle _transparency;
	float _zoom;
	// viewport.origin: scroll offsets
	// viewport.size: allocation size
	public Graphene.Rect viewport;
	// Position and size of the texture node
	public Graphene.Rect texture_box;
	// Visible area of the image
	Graphene.Rect image_box;
	Graphene.Rect selection_box;
	Gtk.Adjustment _hadjustment;
	Gtk.Adjustment _vadjustment;
	bool _first_allocation;
	ulong hadj_changed_id;
	ulong vadj_changed_id;
	uint animation_id;
	ulong current_time;
	uint current_frame;
	uint filter_id;
	bool _paused;
	Gdk.Texture scaled_texture;
	Cancellable filter_cancellable;

	bool dragging;
	bool clicking;
	ClickPoint drag_start;
	ClickPoint last_position;
	Gdk.Cursor prev_cursor = null;

	const float MIN_ZOOM = 0.05f;
	const float MAX_ZOOM = 10.0f;
	const uint ANIMATION_DELAY = 10;
	const uint BIG_IMAGE = 3000 * 3000;
}


class Gth.TextureScaler : Gth.ImageOperation {
	public Graphene.Rect image_box;
	public Graphene.Rect texture_box;
	public ScaleFilter filter;

	public override Gth.Image? execute (Image input, Cancellable cancellable) {
		var visible = input.get_subimage ((uint) image_box.origin.x,
			(uint) image_box.origin.y,
			(uint) image_box.size.width,
			(uint) image_box.size.height);
		if (visible == null) {
			return null;
		}
		var scaled_width = (uint) texture_box.size.width;
		var scaled_height = (uint) texture_box.size.height;
		// stdout.printf ("> VISIBLE: %u, %u\n", visible.get_width (), visible.get_height ());
		// stdout.printf ("  SCALED: %u, %u\n", scaled_width, scaled_height);
		return visible.resize_to (scaled_width, scaled_height,
			filter, cancellable);
	}
}
