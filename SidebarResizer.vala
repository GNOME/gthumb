public class Gth.SidebarResizer : Gtk.Box {
	public signal void started ();
	public signal void changed (double delta);

	public void add_handle () {
		dragging = false;

		var handle = new Gth.ResizeHandle ();
		append (handle);

		var click_events = new Gtk.GestureClick ();
		click_events.pressed.connect ((n_press, x, y) => {
			dragging = true;
			after_click = true;
		});
		click_events.released.connect ((n_press, x, y) => {
			dragging = false;
		});
		handle.add_controller (click_events);

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((x, y) => {
			if (dragging) {
				if (after_click) {
					drag_x = x;
					after_click = false;
					started ();
				}
				else {
					changed (x - drag_x);
				}
			}
		});
		add_controller (motion_events);
	}

	bool dragging;
	double drag_x;
	bool after_click;
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
