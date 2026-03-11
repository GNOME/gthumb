public class Gth.RatingEntry : Gtk.Box {
	public string unchecked_icon_name { set; get; default = "gth-circle-outline-symbolic"; }
	public string checked_icon_name { set; get; default = "gth-circle-filled-symbolic"; }
	public int value {
		get { return _value; }
		set {
			_value = value;
			show_rating (_value);
		}
	}

	int calc_preview (double cursor_x) {
		if (cursor_x == 0) {
			return 0;
		}
		var fraction = (int) Math.floor (cursor_x / star_container.get_width () * (MAX_VALUE + 1));
		return fraction.clamp (0, MAX_VALUE);
	}

	void show_rating (int value) {
		for (var i = 0; i < MAX_VALUE; i++) {
			if (i < value) {
				stars[i].set_from_icon_name (checked_icon_name);
			}
			else {
				stars[i].set_from_icon_name (unchecked_icon_name);
			}
		}
		label.label = "%d".printf (value);
	}

	construct {
		spacing = 0;
		orientation = Gtk.Orientation.HORIZONTAL;
		show_preview = false;

		label = new Gtk.Label ("");
		label.add_css_class ("numeric");
		label.visible = false;
		append (label);

		star_container = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 6);
		star_container.add_css_class ("rating");
		star_container.append (new Gtk.Image ());
		stars = new Gtk.Image[MAX_VALUE];
		for (var i = 0; i < MAX_VALUE; i++) {
			stars[i] = new Gtk.Image.from_icon_name (unchecked_icon_name);
			star_container.append (stars[i]);
		}
		append (star_container);

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((x, y) => {
			if (show_preview) {
				show_rating (calc_preview (x));
			}
			else {
				show_rating (value);
			}
		});
		motion_events.enter.connect (() => {
			show_preview = true;
		});
		motion_events.leave.connect (() => {
			show_rating (value);
		});
		star_container.add_controller (motion_events);

		var click_events = new Gtk.GestureClick ();
		click_events.pressed.connect ((n_press, x, y) => {
			value = calc_preview (x);
		});
		star_container.add_controller (click_events);
	}

	Gtk.Box star_container;
	Gtk.Label label;
	Gtk.Image[] stars;
	bool show_preview;
	int _value;

	const int MAX_VALUE = 5;
}
