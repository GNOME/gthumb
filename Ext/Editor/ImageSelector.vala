public class Gth.ImageSelector : Object, ImageController {
	public signal void changed ();

	public Graphene.Rect selection;
	public GridType grid_type {
		get { return _grid_type; }
		set {
			_grid_type = value;
			if (view != null) {
				view.queue_draw ();
			}
		}
	}
	public float ratio { get { return _ratio; } }
	public int step {
		get {
			return _step;
		}
		set {
			if (value < 1) {
				return;
			}
			_step = value;
			if (snap_enabled ()) {
				Graphene.Rect new_selection = { selection.origin, { 0, 0 } };
				resize_and_set_selection (new_selection, selection.size.width, selection.size.height, AreaType.BOTTOM_RIGHT);
			}
		}
	}

	public ImageSelector (ImageView _view) {
		view = _view;
		view.resized.connect (() => {
			selection_box = to_selection_box (selection);
			update_event_areas ();
		});
		view.scrolled.connect (() => {
			selection_box = to_selection_box (selection);
			update_event_areas ();
		});
		if (view.get_realized ()) {
			on_realize ();
		}
		image_box = { { 0, 0 }, { view.image.width, view.image.height } };

		if (view.image != null) {
			var width = (float) view.image.width;
			var height = (float) view.image.height;
			var sel_width = width / 2;
			var sel_height = height / 2;
			var sel_x = (width - sel_width) / 2;
			var sel_y = (height - sel_height) / 2;
			Graphene.Rect rect = {
				{ sel_x, sel_y },
				{ sel_width, sel_height },
			};
			set_selection (rect);
		}

		var click_events = new Gtk.GestureClick ();
		click_events.button = Gdk.BUTTON_PRIMARY;
		click_events.exclusive = true;
		click_events.pressed.connect ((n_press, x, y) => {
			clicked = true;
			dragging = false;
			click_position = ClickPoint (x, y);
			prev_cursor = view.cursor;
			on_button_press ((float) x, (float) y);
		});
		click_events.released.connect ((n_press, x, y) => {
			clicked = false;
			dragging = false;
			view.cursor = prev_cursor;
			on_button_release ((float) x, (float) y);
		});
		view.add_controller (click_events);

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((x, y) => {
			if (!dragging && clicked) {
				if (click_position.drag_threshold (x, y) && (current_area != null)) {
					dragging = true;
					drag_start = click_position;
					selection_at_drag_start = selection;
					if (current_area.type == AreaType.INNER) {
						view.cursor = new Gdk.Cursor.from_name ("grabbing", null);
					}
				}
			}
			if (dragging) {
				on_dragging ((float) x, (float) y);
			}
			else if (!clicked) {
				on_motion ((float) x, (float) y);
			}
		});
		view.add_controller (motion_events);
	}

	bool ratio_enabled () {
		return _ratio > 0;
	}

	// Try to not change the center of the selection when changing the size.
	void keep_centered (float new_width, float new_height, out float new_x, out float new_y) {
		new_x = selection.origin.x + (selection.size.width - new_width) / 2;
		if (new_x < 0) {
			new_x = 0;
		}
		if (new_x + new_width > image_box.size.width) {
			new_x = image_box.size.width - new_width;
		}
		new_y = selection.origin.y + (selection.size.height - new_height) / 2;
		if (new_y < 0) {
			new_y = 0;
		}
		if (new_y + new_height > image_box.size.height) {
			new_y = image_box.size.height - new_height;
		}
	}

	public void set_ratio (float value, bool swap_sides = false) {
		_ratio = value;
		if (!ratio_enabled ()) {
			return;
		}

		// When changing the aspect ratio, it looks more natural
		// to swap the height and width, rather than keeping
		// the width constant and shrinking the height.
		var new_width = swap_sides ? selection.size.height : selection.size.width;
		var new_height = new_width / _ratio;

		// Try to not change the center of the selection.
		float new_x, new_y;
		keep_centered (new_width, new_height, out new_x, out new_y);

		// stdout.printf ("> TRY: (%f, %f)[%f, %f]\n", new_x, new_y, new_width, new_height);
		Graphene.Rect new_selection = { { new_x, new_y }, { 0, 0 } };
		if (!resize_and_set_selection (new_selection, new_width, new_height, AreaType.BOTTOM_RIGHT)) {
			maximize ();
		}
	}

	public void maximize () {
		var width = image_box.size.width;
		var height = image_box.size.height;
		if (_ratio > 1) {
			width = height * _ratio;
		}
		else if (_ratio > 0) {
			height = width / _ratio;
		}
		Graphene.Rect new_selection = { { 0, 0 }, { 0, 0 } };
		if (resize (ref new_selection, width, height, AreaType.BOTTOM_RIGHT)) {
			// Center
			var new_x = (image_box.size.width - new_selection.size.width) / 2;
			var new_y = (image_box.size.height - new_selection.size.height) / 2;
			new_selection.origin = { new_x, new_y };
			set_selection (new_selection);
		}
	}

	public void center () {
		var new_x = (image_box.size.width - selection.size.width) / 2;
		var new_y = (image_box.size.height - selection.size.height) / 2;
		Graphene.Rect new_selection = { { new_x, new_y }, selection.size };
		set_selection (new_selection);
	}

	public void set_x (float x) {
		if (x == selection.origin.x) {
			return;
		}
		set_selection ({ { x, selection.origin.y }, selection.size });
	}

	public void set_y (float y) {
		if (y == selection.origin.y) {
			return;
		}
		set_selection ({ { selection.origin.x, y }, selection.size });
	}

	public void set_width (float width) {
		if (width == selection.size.width) {
			return;
		}
		set_selection ({ selection.origin, { width, selection.size.height } });
	}

	public void set_height (float height) {
		if (height == selection.size.height) {
			return;
		}
		set_selection ({ selection.origin, { selection.size.width, height } });
	}

	void on_button_press (float x, float y) {
		Graphene.Point point = { x, y };
		if (!view.texture_box.contains_point (point)) {
			return;
		}

		x = Math.roundf ((x - view.texture_box.origin.x) / view.zoom);
		y = Math.roundf ((y - view.texture_box.origin.y) / view.zoom);
		point = { x, y };
		if (selection.contains_point (point)) {
			return;
		}

		if ((current_area == null) || (current_area.type == AreaType.IMAGE)) {
			Graphene.Rect new_selection = { point, { 0, 0 } };
			resize_and_set_selection (new_selection, selector_to_real (10), selector_to_real (10), AreaType.BOTTOM_RIGHT);
			set_active_area (get_area_from_type (AreaType.BOTTOM_RIGHT));
		}
	}

	void respect_image_bounds (Graphene.Rect rect, AreaType area_type, ref float dx, ref float dy) {
		// Min size

		if (area_type.left_side () || area_type.right_side ()) {
			var min_size = (float) _step;
			if (_ratio > 1) {
				min_size = _step * _ratio;
			}
			var min_dx = - (rect.size.width - min_size);
			if (dx < min_dx) {
				dx = min_dx;
			}
		}
		if (area_type.top_side () || area_type.bottom_side ()) {
			var min_size = (float) _step;
			if ((_ratio != 0) && (_ratio < 1)) {
				min_size = _step / _ratio;
			}
			var min_dy = - (rect.size.height - min_size);
			if (dy < min_dy) {
				dy = min_dy;
			}
		}

		// Max size

		if (area_type.right_side ()) {
			var max_dx = image_box.size.width - (rect.origin.x + rect.size.width);
			if (dx > max_dx) {
				dx = max_dx;
			}
		}
		else if (area_type.left_side ()) {
			var min_dx = rect.origin.x;
			if (dx > min_dx) {
				dx = min_dx;
			}
		}
		if (area_type.bottom_side ()) {
			var max_dy = image_box.size.height - (rect.origin.y + rect.size.height);
			if (dy > max_dy) {
				dy = max_dy;
			}
		}
		else if (area_type.top_side ()) {
			var min_dy = rect.origin.y;
			if (dy > min_dy) {
				dy = min_dy;
			}
		}
	}

	void respect_aspect_ratio (Graphene.Rect rect, ref AreaType area_type, ref float dx, ref float dy) {
		if (_ratio == 0) {
			return;
		}
		switch (area_type) {
		case AreaType.TOP:
			dx = dy * _ratio;
			area_type = AreaType.TOP_RIGHT;
			break;

		case AreaType.BOTTOM:
			dx = dy * _ratio;
			area_type = AreaType.BOTTOM_RIGHT;
			break;

		case AreaType.LEFT:
			dy = dx / _ratio;
			area_type = AreaType.BOTTOM_LEFT;
			break;

		case AreaType.RIGHT:
			dy = dx / _ratio;
			area_type = AreaType.BOTTOM_RIGHT;
			break;

		case AreaType.BOTTOM_LEFT, AreaType.BOTTOM_RIGHT, AreaType.TOP_RIGHT, AreaType.TOP_LEFT:
			var new_height = (rect.size.width + dx) / _ratio;
			var new_dy = (new_height - rect.size.height);
			var new_width = (rect.size.height + dy) * _ratio;
			var new_dx = (new_width - rect.size.width);
			if ((rect.size.width + dx) * new_height > (rect.size.height + dy) * new_width) {
				dx = new_dx;
			}
			else {
				dy = new_dy;
			}
			break;
		}
	}

	void respect_step_size (Graphene.Rect rect, AreaType area_type, ref float dx, ref float dy) {
		if (!snap_enabled ()) {
			return;
		}
		if (area_type.left_side () || area_type.right_side ()) {
			var width = rect.size.width + dx;
			var new_width = snap_length (width, _step);
			dx -= (width - new_width);
		}
		if (area_type.top_side () || area_type.bottom_side ()) {
			var height = rect.size.height + dy;
			var new_height = snap_length (height, _step);
			dy -= (height - new_height);
		}
	}

	public bool on_dragging (float x, float y) {
		if ((current_area == null) || (current_area.type == AreaType.IMAGE)) {
			return false;
		}

		var dx = Math.roundf ((x - drag_start.x) / view.zoom);
		var dy = Math.roundf ((y - drag_start.y) / view.zoom);

		// Move the selection.

		if (current_area.type == AreaType.INNER) {
			var new_selection = selection_at_drag_start;
			var new_x = new_selection.origin.x + dx;
			if (new_x < 0) {
				new_x = 0;
			}
			if (new_x + new_selection.size.width > image_box.size.width) {
				new_x = image_box.size.width - new_selection.size.width;
			}
			var new_y = new_selection.origin.y + dy;
			if (new_y < 0) {
				new_y = 0;
			}
			if (new_y + new_selection.size.height > image_box.size.height) {
				new_y = image_box.size.height - new_selection.size.height;
			}
			new_selection.origin = { new_x, new_y };
			set_selection (new_selection);
			return true;
		}

		// Resize the selection.

		// stdout.printf ("> RESIZE: dx: %f\n", dx);
		// stdout.printf ("          dy: %f\n", dy);
		if (current_area.type.left_side ()) {
			dx = -dx;
		}
		if (current_area.type.top_side ()) {
			dy = -dy;
		}
		return resize_and_set_selection (selection_at_drag_start, dx, dy, current_area.type);
	}

	bool resize_and_set_selection (Graphene.Rect rect, float dx, float dy, AreaType area_type) {
		if (!resize (ref rect, dx, dy, area_type)) {
			// stdout.printf ("  NOT!\n");
			return false;
		}
		set_selection (rect);
		return true;
	}

	void grow_upward (ref Graphene.Rect rect, float dy) {
		rect.origin.y = rect.origin.y - dy;
		rect.size.height = rect.size.height + dy;
	}

	void grow_downward (ref Graphene.Rect rect, float dy) {
		rect.size.height = rect.size.height + dy;
	}

	void grow_leftward (ref Graphene.Rect rect, float dx) {
		rect.origin.x = rect.origin.x - dx;
		rect.size.width = rect.size.width + dx;
	}

	void grow_rightward (ref Graphene.Rect rect, float dx) {
		rect.size.width = rect.size.width + dx;
	}

	uint MAX_LOOPS = 10;

	bool resize (ref Graphene.Rect rect, float dx, float dy, AreaType area_type) {
		dx = Math.roundf (dx);
		dy = Math.roundf (dy);
		var loops = 0;
		do {
			float prev_dx = dx, prev_dy = dy;
			respect_step_size (rect, area_type, ref dx, ref dy);
			// stdout.printf (" -[0]-> Area: %s\n", area_type.to_string ());
			// stdout.printf ("        dx: %f\n", dx);
			// stdout.printf ("        dy: %f\n", dy);
			respect_image_bounds (rect, area_type, ref dx, ref dy);
			// stdout.printf (" -[1]-> dx: %f\n", dx);
			// stdout.printf ("        dy: %f\n", dy);
			respect_aspect_ratio (rect, ref area_type, ref dx, ref dy);
			// stdout.printf (" -[2]-> dx: %f\n", dx);
			// stdout.printf ("        dy: %f\n", dy);
			dx = Math.roundf (dx);
			dy = Math.roundf (dy);
			if ((dx == prev_dx) && (dy == prev_dy)) {
				break;
			}
			loops += 1;
		}
		while (loops < MAX_LOOPS);

		 // stdout.printf ("  LOOPS: %d\n", loops);
		if (loops == MAX_LOOPS) {
			// stdout.printf ("  MAX LOOPS\n");
			// Cannot respect all the rules.
			return false;
		}

		if (area_type.top_side ()) {
			grow_upward (ref rect, dy);
		}
		if (area_type.bottom_side ()) {
			grow_downward (ref rect, dy);
		}
		if (area_type.left_side ()) {
			grow_leftward (ref rect, dx);
		}
		if (area_type.right_side ()) {
			grow_rightward (ref rect, dx);
		}
		if (!image_box.contains_rect (rect)) {
			// stdout.printf ("  !CONTAINS\n");
			// stdout.printf ("  delta: (%f, %f)\n", dx, dy);
			// stdout.printf ("  rect: (%f, %f) [%f, %f]\n",
			// 	rect.origin.x,
			// 	rect.origin.y,
			// 	rect.size.width,
			// 	rect.size.height);
			// stdout.printf ("  image_box: (%f, %f) [%f, %f]\n",
			// 	image_box.origin.x,
			// 	image_box.origin.y,
			// 	image_box.size.width,
			// 	image_box.size.height);
			return false;
		}
		//set_selection (rect);
		return true;
	}

	void on_motion (float x, float y) {
		Graphene.Point point = { x, y };
		set_active_area (get_area_at_point (point));
		view.queue_draw ();
	}

	void on_button_release (float x, float y) {
		current_area = null;
	}

	void set_active_area (EventArea? area) {
		active = (area != null);
		current_area = area;
		view.cursor = (current_area != null) ? current_area.cursor : null;
	}

	public bool can_snap () {
		return (_ratio == 0)
			|| (_ratio == 1)
			// _ratio is integer:
			|| (Math.floorf (_ratio) == _ratio)
			// _ratio reciprocal is integer:
			|| (Math.floorf (1f / _ratio) == 1f / _ratio);
	}

	bool snap_enabled () {
		return (_step != 1) && can_snap ();
	}

	float snap_length (float length, int size) {
		return Math.roundf (((int) length / size) * size);
		/*
		var previous = ((int) length / size) * size;
		var next = previous + size;
		if (length - previous <= (float) next - length) {
			length = previous;
		}
		else {
			length = next;
		}
		return length;*/
	}

	float real_to_selector (float value) {
		return Math.roundf (view.zoom * value);
	}

	float selector_to_real (float value) {
		return Math.roundf (value / view.zoom);
	}

	Graphene.Rect to_selection_box (Graphene.Rect real_area) {
		return {
			{ real_to_selector (real_area.origin.x) - view.viewport.origin.x , real_to_selector (real_area.origin.y) - view.viewport.origin.y },
			{ real_to_selector (real_area.size.width) , real_to_selector (real_area.size.height) },
		};
	}

	Graphene.Rect to_real_area (Graphene.Rect selection_box) {
		return {
			{ selector_to_real (selection_box.origin.x + view.viewport.origin.x) , selector_to_real (selection_box.origin.y + view.viewport.origin.y) },
			{ selector_to_real (selection_box.size.width) , selector_to_real (selection_box.size.height) },
		};
	}

	EventArea? get_area_at_point (Graphene.Point point) {
		foreach (var area in areas) {
			if (area.rect.contains_point (point)) {
				return area;
			}
		}
		return null;
	}

	EventArea? get_area_from_type (AreaType type) {
		foreach (var area in areas) {
			if (area.type == type) {
				return area;
			}
		}
		return null;
	}

	const float DEFAULT_BORDER = 40;
	const float MIN_BORDER = 3;

	void update_event_areas () {
		var border = DEFAULT_BORDER;
		if (selection_box.size.width < DEFAULT_BORDER * 3) {
			border = selection_box.size.width / 3;
		}
		if (selection_box.size.height < DEFAULT_BORDER * 3) {
			border = selection_box.size.height / 3;
		}
		if (border < MIN_BORDER) {
			border = MIN_BORDER;
		}

		var border_2 = border * 2;

		var x = view.texture_box.origin.x + selection_box.origin.x;
		var y = view.texture_box.origin.y + selection_box.origin.y;
		var width = selection_box.size.width;
		var height = selection_box.size.height;

		var area = get_area_from_type (AreaType.INNER);
		area.rect = { { x, y }, { width, height } };

		area = get_area_from_type (AreaType.TOP);
		area.rect = { { x + border, y }, { width - border_2, border } };

		area = get_area_from_type (AreaType.BOTTOM);
		area.rect = { { x + border, y + height - border }, { width - border_2, border } };

		area = get_area_from_type (AreaType.LEFT);
		area.rect = { { x, y + border }, { border, height - border_2 } };

		area = get_area_from_type (AreaType.RIGHT);
		area.rect = { { x + width - border, y + border }, { border, height - border_2 } };

		area = get_area_from_type (AreaType.TOP_LEFT);
		area.rect = { { x, y }, { border, border } };

		area = get_area_from_type (AreaType.TOP_RIGHT);
		area.rect = { { x + width - border, y }, { border, border } };

		area = get_area_from_type (AreaType.BOTTOM_LEFT);
		area.rect = { { x, y + height - border }, { border, border } };

		area = get_area_from_type (AreaType.BOTTOM_RIGHT);
		area.rect = { { x + width - border, y + height - border }, { border, border } };

		area = get_area_from_type (AreaType.IMAGE);
		area.rect = view.texture_box;
	}

	void selection_changed () {
		selection_box = to_selection_box (selection);
		update_event_areas ();
		changed ();
		view.queue_draw ();
	}

	void set_selection (Graphene.Rect rect, bool force_update = false) {
		rect = {
			{ Math.roundf (rect.origin.x), Math.roundf (rect.origin.y) },
			{ Math.floorf (rect.size.width), Math.floorf (rect.size.height) }
		};
		if (!force_update && rect.equal (selection)) {
			return;
		}
		selection = rect;
		// stdout.printf ("> selection: (%f, %f) [%f, %f]\n",
		// 	selection_box.origin.x,
		// 	selection_box.origin.y,
		// 	selection_box.size.width,
		// 	selection_box.size.height);
		selection_changed ();
	}

	void init_selection () {
		Graphene.Rect zero = { { 0, 0 }, { 0, 0 } };
		set_selection (zero, false);
	}

	public void on_realize () {
		areas = {};
		areas += new EventArea (AreaType.TOP, "n-resize");
		areas += new EventArea (AreaType.BOTTOM, "s-resize");
		areas += new EventArea (AreaType.LEFT, "w-resize");
		areas += new EventArea (AreaType.RIGHT, "e-resize");
		areas += new EventArea (AreaType.TOP_LEFT, "nw-resize");
		areas += new EventArea (AreaType.TOP_RIGHT, "ne-resize");
		areas += new EventArea (AreaType.BOTTOM_LEFT, "sw-resize");
		areas += new EventArea (AreaType.BOTTOM_RIGHT, "se-resize");
		areas += new EventArea (AreaType.INNER, "grab");
		areas += new EventArea (AreaType.IMAGE, "crosshair");
		current_area = null;
		init_selection ();
		update_event_areas ();
	}

	public void on_unrealize () {
		areas = {};
	}

	public void on_size_allocated () {
		selection_changed ();
	}

	public void on_snapshot (Gtk.Snapshot snapshot) {
		// stdout.printf ("> selection_box: (%f, %f) [%f, %f]\n",
		// 	selection_box.origin.x,
		// 	selection_box.origin.y,
		// 	selection_box.size.width,
		// 	selection_box.size.height);

		// Darker image.

		var box = selection_box;
		box.origin.x += view.texture_box.origin.x;
		box.origin.y += view.texture_box.origin.y;

		Gdk.RGBA background_color = { 0, 0, 0, 0.33f };
		var round_x =  Math.roundf (box.origin.x);
		snapshot.append_color (background_color, {
			{ 0, 0 },
			{ round_x, view.viewport.size.height }
		});
		var round_width = Math.roundf (box.size.width);
		snapshot.append_color (background_color, {
			{ round_x + round_width, 0 },
			{ view.viewport.size.width - (round_x + round_width) - 0.5f, view.viewport.size.height }
		});
		var round_y = Math.roundf (box.origin.y);
		snapshot.append_color (background_color, {
			{ round_x, 0 },
			{ round_width, round_y }
		});
		var round_height = Math.roundf (box.size.height);
		snapshot.append_color (background_color, {
			{ round_x, round_y + round_height },
			{ round_width, view.viewport.size.height - (round_y + round_height) }
		});

		// Selection box

		if ((selection_box.size.width == 0) || (selection_box.size.height == 0)) {
			return;
		}

		var rect = new Gsk.RoundedRect ();
		rect.init_from_rect (box, 0);
		Gdk.RGBA border_color = { 1, 1, 1, 1 };
		var border_width = 2f;
		snapshot.append_border (rect,
			{ border_width, border_width, border_width, border_width },
			{ border_color, border_color, border_color, border_color }
		);

		if ((current_area != null) && (current_area.type != AreaType.INNER)) {
			rect = new Gsk.RoundedRect ();
			rect.init_from_rect (current_area.rect, 0);
			border_color = { 1, 1, 1, 1 };

			float[] borders_width;
			switch (current_area.type) {
			case AreaType.BOTTOM:
				borders_width = { 2, 2, 0, 2 };
				break;
			case AreaType.TOP:
				borders_width = { 0, 2, 2, 2 };
				break;
			case AreaType.LEFT:
				borders_width = { 2, 2, 2, 0 };
				break;
			case AreaType.RIGHT:
				borders_width = { 2, 0, 2, 2 };
				break;
			case AreaType.BOTTOM_LEFT:
				borders_width = { 2, 2, 0, 0 };
				break;
			case AreaType.BOTTOM_RIGHT:
				borders_width = { 2, 0, 0, 2 };
				break;
			case AreaType.TOP_LEFT:
				borders_width = { 0, 2, 2, 0 };
				break;
			case AreaType.TOP_RIGHT:
				borders_width = { 0, 0, 2, 2 };
				break;
			default:
				borders_width = { 0, 0, 0, 0 };
				break;
			}

			snapshot.append_border (rect, borders_width,
				{ border_color, border_color, border_color, border_color }
			);
		}

		if (_grid_type != GridType.NONE) {
			append_grid (snapshot, box);
		}
	}

	void append_grid (Gtk.Snapshot snapshot, Graphene.Rect rect) {
		Gdk.RGBA color = { 1, 1, 1, 1f };
		var stroke = new Gsk.Stroke (0.75f);
		Gdk.RGBA thin_color = { 1, 1, 1, 0.3f };
		var thin_stroke = new Gsk.Stroke (0.5f);
		var builder = new Gsk.PathBuilder ();

		switch (_grid_type) {
		case GridType.RULE_OF_THIRDS:
			for (var i = 1; i < 3; i++) {
				builder.move_to (rect.origin.x + rect.size.width * i / 3, rect.origin.y);
				builder.line_to (rect.origin.x + rect.size.width * i / 3, rect.origin.y + rect.size.height);

				builder.move_to (rect.origin.x, rect.origin.y + rect.size.height * i / 3);
				builder.line_to (rect.origin.x + rect.size.width, rect.origin.y + rect.size.height * i / 3);
			}
			snapshot.append_stroke (builder.to_path (), stroke, color);

			builder = new Gsk.PathBuilder ();
			for (var i = 1; i < 9; i++) {
				if (i % 3 == 0) {
					continue;
				}

				builder.move_to (rect.origin.x + rect.size.width * i / 9, rect.origin.y);
				builder.line_to (rect.origin.x + rect.size.width * i / 9, rect.origin.y + rect.size.height);

				builder.move_to (rect.origin.x, rect.origin.y + rect.size.height * i / 9);
				builder.line_to (rect.origin.x + rect.size.width, rect.origin.y + rect.size.height * i / 9);
			}
			snapshot.append_stroke (builder.to_path (), thin_stroke, thin_color);
			break;

		case GridType.GOLDEN_RATIO:
			var x_delta = rect.size.width * GOLDER_RATIO_FACTOR;
			var y_delta = rect.size.height * GOLDER_RATIO_FACTOR;
			builder.move_to (rect.origin.x + x_delta, rect.origin.y);
			builder.line_to (rect.origin.x + x_delta, rect.origin.y + rect.size.height);
			if (x_delta < rect.size.width / 2) {
				builder.move_to (rect.origin.x + rect.size.width - x_delta, rect.origin.y);
				builder.line_to (rect.origin.x + rect.size.width - x_delta , rect.origin.y + rect.size.height);
			}
			builder.move_to (rect.origin.x, rect.origin.y + y_delta);
			builder.line_to (rect.origin.x + rect.size.width, rect.origin.y + y_delta);
			if (y_delta < rect.size.height / 2) {
				builder.move_to (rect.origin.x, rect.origin.y + rect.size.height - y_delta);
				builder.line_to (rect.origin.x + rect.size.width, rect.origin.y + rect.size.height - y_delta);
			}
			snapshot.append_stroke (builder.to_path (), stroke, color);
			break;

		case GridType.CENTER_LINES:
			builder.move_to (rect.origin.x + rect.size.width / 2, rect.origin.y);
			builder.line_to (rect.origin.x + rect.size.width / 2, rect.origin.y + rect.size.height);
			builder.move_to (rect.origin.x, rect.origin.y + rect.size.height / 2);
			builder.line_to (rect.origin.x + rect.size.width, rect.origin.y + rect.size.height / 2);
			snapshot.append_stroke (builder.to_path (), stroke, color);

			builder = new Gsk.PathBuilder ();
			for (var i = 1; i < 4; i++) {
				if (i == 2) {
					continue;
				}
				builder.move_to (rect.origin.x + rect.size.width * i / 4, rect.origin.y);
				builder.line_to (rect.origin.x + rect.size.width * i / 4, rect.origin.y + rect.size.height);
				builder.move_to (rect.origin.x, rect.origin.y + rect.size.height * i / 4);
				builder.line_to (rect.origin.x + rect.size.width, rect.origin.y + rect.size.height * i / 4);
			}
			snapshot.append_stroke (builder.to_path (), thin_stroke, thin_color);
			break;

		case GridType.UNIFORM:
			color = { 1, 1, 1, 0.6f };
			stroke = new Gsk.Stroke (0.75f);
			for (var x = GRID_STEP; x < rect.size.width; x += GRID_STEP) {
				builder.move_to (rect.origin.x + x, rect.origin.y);
				builder.line_to (rect.origin.x + x, rect.origin.y + rect.size.height);
			}
			for (var y = GRID_STEP; y < rect.size.height; y += GRID_STEP) {
				builder.move_to (rect.origin.x, rect.origin.y + y);
				builder.line_to (rect.origin.x + rect.size.width, rect.origin.y + y);
			}
			snapshot.append_stroke (builder.to_path (), stroke, color);
			break;

		default:
			break;
		}
	}

	class EventArea {
		public AreaType type;
		public Gdk.Cursor cursor;
		public Graphene.Rect rect;

		public EventArea (AreaType _type, string cursor_name) {
			type = _type;
			cursor = new Gdk.Cursor.from_name (cursor_name, null);
			rect = { { 0, 0 }, { 0, 0 } };
		}
	}

	enum AreaType {
		TOP,
		BOTTOM,
		LEFT,
		RIGHT,
		TOP_LEFT,
		TOP_RIGHT,
		BOTTOM_LEFT,
		BOTTOM_RIGHT,
		INNER,
		IMAGE;

		public bool left_side () {
			return (this == AreaType.LEFT)
				|| (this == AreaType.TOP_LEFT)
				|| (this == AreaType.BOTTOM_LEFT);
		}

		public bool right_side () {
			return (this == AreaType.RIGHT)
				|| (this == AreaType.TOP_RIGHT)
				|| (this == AreaType.BOTTOM_RIGHT);
		}

		public bool top_side () {
			return (this == AreaType.TOP)
				|| (this == AreaType.TOP_LEFT)
				|| (this == AreaType.TOP_RIGHT);
		}

		public bool bottom_side () {
			return (this == AreaType.BOTTOM)
				|| (this == AreaType.BOTTOM_LEFT)
				|| (this == AreaType.BOTTOM_RIGHT);
		}
	}

	construct {
		_ratio = 0;
		_step = 1;
		_grid_type = GridType.RULE_OF_THIRDS;
		active = false;
	}

	Graphene.Rect selection_box;
	Graphene.Rect image_box;
	ImageView view;
	int _step;
	float _ratio;
	EventArea[] areas;
	EventArea current_area;
	bool active;
	bool clicked;
	bool dragging;
	Gdk.Cursor prev_cursor;
	ClickPoint click_position;
	ClickPoint drag_start;
	Graphene.Rect selection_at_drag_start;
	GridType _grid_type;

	const float GOLDEN_RATIO = 1.6180339887f;
	const float GOLDER_RATIO_FACTOR = (GOLDEN_RATIO / (1.0f + 2.0f * GOLDEN_RATIO));
	const float GRID_STEP = 100f;
	const int MAX_GRID_LINES = 50;
}
