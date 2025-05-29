public class Gth.TimeSelector : Gtk.Box {
	public enum Mode {
		WITHOUT_TIME,
		OPTIONAL_TIME,
	}

	public signal void changed ();
	public Gth.Date selected_date;
	public Gth.Time selected_time;

	public TimeSelector (Mode _mode) {
		orientation = Gtk.Orientation.HORIZONTAL;
		spacing = HORIZONTAL_SPACING;
		mode = _mode;
		selected_date = Gth.Date ();
		selected_time = Gth.Time ();

		date_entry = new Gtk.Entry ();
		date_entry.set_icon_from_icon_name (Gtk.EntryIconPosition.SECONDARY, "calendar-symbolic");
		date_entry.width_chars = 10;
		date_entry.icon_press.connect ((icon_pos) => {
			if (icon_pos == Gtk.EntryIconPosition.SECONDARY) {
				calendar_popup.popup ();
			}
		});
		append (date_entry);

		calendar = new Gtk.Calendar ();
		calendar.day_selected.connect ((_calendar) => {
			var datetime = _calendar.get_date ();
			selected_date = Date.from_gdatetime (datetime);
			update_entry_from_data ();
			changed ();
		});

		var calendar_box = new Gtk.Box (Gtk.Orientation.VERTICAL, 6);
		calendar_box.append (calendar);

		calendar_popup = new Gtk.Popover ();
		calendar_popup.child = calendar_box;
		append (calendar_popup);

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

	public void set_time (Gth.DateTime? datetime) {
		if (datetime != null) {
			selected_date = datetime.date;
			selected_time = datetime.time;
		}
		else {
			selected_date = Gth.Date ();
			selected_time = Gth.Time ();
		}
		update_entry_from_data ();
		if (selected_date.is_valid ()) {
			calendar.select_day (selected_date.to_local_gdatetime ());
		}
		changed ();
	}

	public Gth.DateTime? get_time () {
		var datetime = new Gth.DateTime ();
		datetime.date = selected_date;
		datetime.time = selected_time;
		return datetime;
	}

	public void focus () {
		date_entry.grab_focus ();
	}

	void update_entry_from_data () {
		date_entry.text = selected_date.to_display_string ();
	}

	Gtk.Entry date_entry;
	Gtk.CheckButton time_checkbutton;
	Gtk.Entry time_entry;
	Gtk.Popover calendar_popup;
	Gtk.Calendar calendar;
	Mode mode;
}
