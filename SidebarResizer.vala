public class Gth.SidebarResizer : Gtk.Box {
	public signal void started ();
	public signal void ended ();

	public void add_handle (Adw.OverlaySplitView _view, Gtk.PackType _pack_type) {
		view = _view;
		pack_type = _pack_type;
		dragging = false;

		var handle = new Gth.ResizeHandle ();
		if (pack_type == Gtk.PackType.END)
			append (handle);
		else
			prepend (handle);

		var click_events = new Gtk.GestureClick ();
		click_events.pressed.connect ((n_press, x, y) => {
			dragging = true;
			after_click = true;
		});
		click_events.released.connect ((n_press, x, y) => {
			dragging = false;
			ended ();
		});
		handle.add_controller (click_events);

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((x, y) => {
			if (dragging) {
				if (after_click) {
					after_click = false;
					set_started (x);
				}
			}
		});
		add_controller (motion_events);
	}

	public void update_width (double abs_drag_x) {
		if (pack_type == Gtk.PackType.END) {
			view.max_sidebar_width = initial_width + (abs_drag_x - abs_start_x);
		}
		else {
			view.max_sidebar_width = initial_width + (abs_start_x - abs_drag_x);
		}
	}

	void set_started (double drag_x) {
		initial_width = view.max_sidebar_width;
		int resizer_x;
		Util.get_widget_abs_position (this, out resizer_x, null);
		abs_start_x = resizer_x + drag_x;
		started ();
	}

	Adw.OverlaySplitView view;
	Gtk.PackType pack_type;
	bool dragging;
	bool after_click;
	double initial_width;
	double abs_start_x;
}

class Gth.ResizeHandle : Gtk.Widget {
	construct {
		add_css_class ("resizer");
		cursor = new Gdk.Cursor.from_name ("col-resize", null);
	}

	public override Gtk.SizeRequestMode get_request_mode () {
		return Gtk.SizeRequestMode.HEIGHT_FOR_WIDTH;
	}

	public override void measure (Gtk.Orientation orientation, int for_size, out int minimum, out int natural, out int minimum_baseline, out int natural_baseline) {
		minimum = 0;
		natural = 0;
		natural_baseline = -1;
		minimum_baseline = -1;

		if (orientation == Gtk.Orientation.HORIZONTAL) {
			minimum = 6;
			natural = minimum;
		}
		else {
			minimum = -1;
			natural = for_size;
		}
	}
}
