public class Gth.TimeRow : Adw.EntryRow {
	public Gth.Time time {
		get {
			return Time.parse (text);
		}

		set {
			text = value.to_display_string ();
		}
	}

	public string exif_date {
		set {
			var exif_datetime = DateTime.get_from_exif_date (value);
			text = (exif_datetime != null) ? exif_datetime.time.to_display_string () : null;
		}
	}

	public string get_exif_date () {
		return time.to_exif_date ();
	}

	construct {
		var now_button = new Gtk.Button.from_icon_name ("gth-clock-symbolic");
		now_button.tooltip_text = _("Set to current time");
		now_button.valign = Gtk.Align.CENTER;
		now_button.add_css_class ("flat");
		now_button.clicked.connect (() => {
			var now = new GLib.DateTime.now_local ();
			time = Gth.Time.from_gdatetime (now);
		});
		add_suffix (now_button);
	}
}
