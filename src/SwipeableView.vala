public class Gth.SwipeableView : Gtk.Widget, Gtk.Buildable {
	public signal void change_content (Gth.NavigationDirection direction);

	public signal void can_change_content (Gth.NavigationDirection direction, out bool can_change) {
		can_change = true;
	}

	public signal void scroll_begin ();

	public signal void scroll_end ();

	public Gtk.Widget child {
		get { return _child; }
		set {
			if (_child != null) {
				_child.unparent ();
			}
			_child = value;
			if (_child != null) {
				_child.insert_before (this, preloaded_left);
				queue_resize ();
			}
		}
	}

	public bool enabled {
		get { return _enabled; }
		set {
			_enabled = value;
			update_controllers ();
		}
	}

	public bool allow_mouse_drag {
		get { return _allow_mouse_drag; }
		set {
			_allow_mouse_drag = value;
			update_controllers ();
		}
	}

	public override Gtk.SizeRequestMode get_request_mode () {
		return Gtk.SizeRequestMode.HEIGHT_FOR_WIDTH;
	}

	public override void measure (Gtk.Orientation orientation, int for_size, out int minimum, out int natural, out int minimum_baseline, out int natural_baseline) {
		if ((_child == null) || !_child.visible) {
			minimum = 0;
			natural = 0;
			minimum_baseline = -1;
			natural_baseline = -1;
			return;
		}
		_child.measure (orientation, for_size, out minimum,
			out natural, out minimum_baseline, out natural_baseline);
	}

	public override void size_allocate (int width, int height, int baseline) {
		if (_child == null) {
			return;
		}

		Gsk.Transform transform = null;
		if (_swipe_offset != 0) {
			transform = new Gsk.Transform ().translate ({ _swipe_offset, 0 });
		}
		_child.allocate (width, height, baseline, transform);

		const int margin = 32;
		int preloaded_width;

		preloaded_left.measure (Gtk.Orientation.VERTICAL, height, null, out preloaded_width, null, null);
		transform = new Gsk.Transform ().translate ({ margin, 0 });
		preloaded_left.allocate (preloaded_width, height, baseline, transform);

		preloaded_right.measure (Gtk.Orientation.VERTICAL, height, null, out preloaded_width, null, null);
		transform = new Gsk.Transform ().translate ({ width - preloaded_width - margin, 0 });
		preloaded_right.allocate (preloaded_width, height, baseline, transform);
	}

	public void add_child (Gtk.Builder builder, Object child, string? type) {
		this.child = child as Gtk.Widget;
	}

	public override void dispose () {
		child = null;
		preloaded_left.unparent ();
		preloaded_right.unparent ();
		base.dispose ();
	}

	public void stop_swipe_gesture () {
		drag_gesture.set_state (Gtk.EventSequenceState.DENIED);
		swipe_gesture.set_state (Gtk.EventSequenceState.DENIED);
	}

	void init_can_change_content () {
		can_change_content (NavigationDirection.FORWARD, out can_change_next);
		can_change_content (NavigationDirection.BACK, out can_change_previous);
	}

	float resistence (double dx) {
		var sign = (dx < 0) ? -1 : 1;
		var max_x = child.get_width () / 2;
		var x = (dx.abs () / max_x) * 10;
		double y;
		var can_navigate = ((NavigationDirection.from_sign (sign) == NavigationDirection.FORWARD) && can_change_next)
			|| ((NavigationDirection.from_sign (sign) == NavigationDirection.BACK) && can_change_previous);
		if (can_navigate) {
			// Less resistence: linear from 0 to 4 then logarithmic, reaching 6 when x is 10.
			y = (x <= 4) ? x : Math.log (x / 1.42 + 1) * 3;
		}
		else {
			// More resistence: always logarithmic, reaching 1 when x is 10.
			y = Math.log (x + 1) * 0.5;
		}
		dx = (y / 10) * max_x;
		return (float) (dx * sign);
	}

	float swipe_offset {
		set {
			_swipe_offset = value;
			queue_allocate ();
		}
	}

	void update_controllers () {
		if (_enabled) {
			touchpad_events.propagation_phase = Gtk.PropagationPhase.BUBBLE;
		}
		else {
			touchpad_events.propagation_phase = Gtk.PropagationPhase.NONE;
		}
		if (_enabled && _allow_mouse_drag) {
			swipe_gesture.propagation_phase = Gtk.PropagationPhase.BUBBLE;
			drag_gesture.propagation_phase = Gtk.PropagationPhase.CAPTURE;
		}
		else {
			swipe_gesture.propagation_phase = Gtk.PropagationPhase.NONE;
			drag_gesture.propagation_phase = Gtk.PropagationPhase.NONE;
		}
	}

	bool is_long_swipe (double delta) {
		return delta.abs () >= double.min ((1.0 / 4.0) * child.get_width (), LONG_SWIPE);
	}

	void show_preloaded (NavigationDirection dir) {
		preloaded_left.set_child_visible (dir == NavigationDirection.BACK);
		preloaded_right.set_child_visible (dir == NavigationDirection.FORWARD);
		queue_allocate ();
	}

	void hide_preloaded () {
		preloaded_left.set_child_visible (false);
		preloaded_right.set_child_visible (false);
		queue_allocate ();
	}

	construct {
		preloaded_left = new Gtk.Image.from_icon_name ("go-previous-symbolic");
		preloaded_left.icon_size = Gtk.IconSize.LARGE;
		preloaded_left.valign = Gtk.Align.FILL;
		preloaded_left.set_parent (this);
		preloaded_left.set_child_visible (false);

		preloaded_right = new Gtk.Image.from_icon_name ("go-next-symbolic");
		preloaded_right.icon_size = Gtk.IconSize.LARGE;
		preloaded_right.valign = Gtk.Align.FILL;
		preloaded_right.insert_before (this, null);
		preloaded_right.set_child_visible (false);

		// Touchpad Swipe

		touchpad_events = new Gtk.EventControllerScroll (Gtk.EventControllerScrollFlags.HORIZONTAL | Gtk.EventControllerScrollFlags.KINETIC);
		touchpad_events.scroll_begin.connect ((controller) => {
			touchpad_offset = 0;
			init_can_change_content ();
			scroll_begin ();
		});
		touchpad_events.scroll.connect ((controller, dx, dy) => {
			if (!Util.smooth_scroll_from_touchpad (controller)) {
				return false;
			}
			touchpad_offset += dx;
			swipe_offset = resistence (touchpad_offset);
			if (is_long_swipe (_swipe_offset)) {
				show_preloaded (NavigationDirection.from_sign (_swipe_offset));
			}
			else {
				hide_preloaded ();
			}
			return true;
		});
		touchpad_events.decelerate.connect ((controller, vel_x, vel_y) => {
			// stdout.printf ("> touchpad_offset: %f", touchpad_offset);
			// stdout.printf ("  vel_x: %f", vel_x);
			if ((touchpad_offset.abs () >= MIN_SWIPE) && (vel_x.abs () >= MIN_VELOCITY)) {
				change_content (NavigationDirection.from_sign (vel_x));
			}
			else if (is_long_swipe (touchpad_offset)) {
				change_content (NavigationDirection.from_sign (touchpad_offset));
			}
			touchpad_offset = 0;
			swipe_offset = 0;
			hide_preloaded ();
			scroll_end ();
		});
		add_controller (touchpad_events);

		// Drag events -> swipe content

		drag_gesture = new Gtk.GestureDrag ();
		drag_gesture.exclusive = true;
		var drag_begin_id = drag_gesture.drag_begin.connect ((controller, start_x, start_y) => {
			var state = controller.get_current_event_state ();
			if (Gdk.ModifierType.CONTROL_MASK in state) {
				controller.set_state (Gtk.EventSequenceState.DENIED);
				return;
			}
			init_can_change_content ();
			scroll_begin ();
		});
		var drag_update_id = drag_gesture.drag_update.connect ((controller, offset_x, offset_y) => {
			swipe_offset = resistence (offset_x);
			if (is_long_swipe (_swipe_offset)) {
				var dir = (_swipe_offset > 0) ? NavigationDirection.BACK : NavigationDirection.FORWARD;
				show_preloaded (dir);
			}
			else {
				hide_preloaded ();
			}
		});
		var drag_end_id = drag_gesture.drag_end.connect ((controller, offset_x, offset_y) => {
			// stdout.printf ("> drag_end: offset_x: %f\n", offset_x.abs ());
			swipe_offset = 0;
			hide_preloaded ();
			scroll_end ();
			if (offset_x.abs () < MIN_SWIPE) {
				swipe_gesture.set_state (Gtk.EventSequenceState.DENIED);
			}
			if (is_long_swipe (resistence (offset_x))) {
				swipe_gesture.set_state (Gtk.EventSequenceState.DENIED);
				change_content ((offset_x > 0) ? NavigationDirection.BACK : NavigationDirection.FORWARD);
			}
		});
		add_controller (drag_gesture);

		// Swipe -> change file

		swipe_gesture = new Gtk.GestureSwipe ();
		var swipe_id = swipe_gesture.swipe.connect ((vel_x, vel_y) => {
			// stdout.printf ("> swipe: vel_x: %f\n", vel_x.abs ());
			if (vel_x.abs () >= MIN_VELOCITY) {
				scroll_end ();
				change_content ((vel_x > 0) ? NavigationDirection.BACK : NavigationDirection.FORWARD);
			}
		});
		add_controller (swipe_gesture);

		_allow_mouse_drag = false;
		_enabled = false;
		update_controllers ();
	}

	Gtk.Widget _child;
	Gtk.Image preloaded_left;
	Gtk.Image preloaded_right;
	float _swipe_offset;
	double touchpad_offset;
	Gtk.EventControllerScroll touchpad_events;
	Gtk.GestureDrag drag_gesture;
	Gtk.GestureSwipe swipe_gesture;
	bool _enabled;
	bool _allow_mouse_drag;
	bool can_change_next;
	bool can_change_previous;

	const double MIN_VELOCITY = 100;
	const double MIN_SWIPE = 100;
	const double LONG_SWIPE = 300;
}
