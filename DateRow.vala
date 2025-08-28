public class Gth.DateRow : Adw.EntryRow {
	public Gth.Date date {
		get {
			return Date.parse (text);
		}

		set {
			text = value.to_display_string ();
		}
	}

	public string exif_date {
		set {
			var exif_datetime = DateTime.get_from_exif_date (value);
			text = (exif_datetime != null) ? exif_datetime.date.to_display_string () : null;
		}
	}

	public string get_exif_date () {
		return date.to_exif_date ();
	}

	construct {
		var calendar = new Gtk.Calendar ();
		calendar.day_selected.connect ((_calendar) => {
			var datetime = _calendar.get_date ();
			var selected_date = Date.from_gdatetime (datetime);
			text = selected_date.to_display_string ();
		});

		var calendar_popup = new Gtk.Popover ();
		calendar_popup.child = calendar;

		var calendar_button = new Gtk.MenuButton ();
		calendar_button.icon_name = "gth-calendar-symbolic";
		calendar_button.tooltip_text = _("Select a day");
		calendar_button.popover = calendar_popup;
		calendar_button.valign = Gtk.Align.CENTER;
		calendar_button.add_css_class ("flat");
		add_suffix (calendar_button);

		var now_button = new Gtk.Button.from_icon_name ("gth-clock-symbolic");
		now_button.tooltip_text = _("Set to today");
		now_button.valign = Gtk.Align.CENTER;
		now_button.add_css_class ("flat");
		now_button.clicked.connect (() => {
			var now = new GLib.DateTime.now_local ();
			var date = Gth.Date.from_gdatetime (now);
			text = date.to_display_string ();
		});
		add_suffix (now_button);
	}
}
