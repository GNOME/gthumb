public class Gth.TimeSelector : Gtk.Box {
	public enum Mode {
		WITHOUT_TIME,
		OPTIONAL_TIME,
	}

	public signal void changed ();

	public TimeSelector (Mode _mode) {
		orientation = Gtk.Orientation.HORIZONTAL;
		spacing = HORIZONTAL_SPACING;
		mode = _mode;

		date_entry = new Gtk.Entry ();
		date_entry.width_chars = 10;
		append (date_entry);

		if (mode != WITHOUT_TIME) {
			if (mode == OPTIONAL_TIME) {
				time_checkbutton = new Gtk.CheckButton ();
				append (time_checkbutton);
			}

			time_entry = new Gtk.Entry ();
			time_entry.width_chars = 10;
			append (time_entry);
		}
	}

	public void set_time (Gth.DateTime? time) {
		// TODO
	}

	public Gth.DateTime? get_time () {
		// TODO
		return null;
	}

	public void focus () {
		date_entry.grab_focus ();
	}

	Gtk.Entry date_entry;
	Gtk.CheckButton time_checkbutton;
	Gtk.Entry time_entry;
	Mode mode;
}
