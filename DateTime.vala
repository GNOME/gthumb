public class Gth.DateTime {
	public Gth.Date date;
	public Gth.Time time;

	public DateTime () {
		clear ();
	}

	public DateTime.from_date_time (Gth.Date _date, Gth.Time _time) {
		date = _date;
		time = _time;
	}

	public DateTime.from_ymd_hms (int _year, int _month, int _day, int _hour, int _min, int _second, uint _usecond = 0) {
		date = Gth.Date.from_ymd ((uint) _year, (uint8) _month, (uint8) _day);
		time = Gth.Time.from_hms ((uint8) _hour, (uint8) _min, (uint8) _second, (uint) _usecond);
	}

	public DateTime.from_gdate (GLib.Date gdate) {
		date = Gth.Date.from_ymd (gdate.get_year (), gdate.get_month (), gdate.get_day ());
		time = Gth.Time ();
	}

	public bool is_valid () {
		return date.is_valid () && time.is_valid ();
	}

	public bool date_is_valid () {
		return date.is_valid ();
	}

	public void clear () {
		date = Gth.Date ();
		time = Gth.Time ();
	}

	public string to_exif_date () {
		if (is_valid ()) {
			return "%4u:%02u:%02u %02u:%02u:%02u".printf (
				date.year, date.month, date.day,
				time.hour, time.minute, time.second
			);
		}
		else {
			return date.to_exif_date ();
		}
	}

	public void copy (Gth.DateTime other) {
		date = other.date;
		time = other.time;
	}

	public bool set_from_exif_date (string? exif_date) {
		var exif_datetime = DateTime.get_from_exif_date (exif_date);
		if (exif_datetime != null) {
			copy (exif_datetime);
			return true;
		}
		else {
			clear ();
			return false;
		}
	}

	public void set_from_gdatetime (GLib.DateTime gtime) {
		date = Gth.Date.from_ymd (gtime.get_year (), (uint8) gtime.get_month (), (uint8) gtime.get_day_of_month ());
		time = Gth.Time.from_hms ((uint8) gtime.get_hour (), (uint8) gtime.get_minute (), (uint8) gtime.get_second ());
	}

	public GLib.DateTime? to_local_gtime () {
		if (is_valid ()) {
			return new GLib.DateTime (
				new TimeZone.local (),
				(int) date.year, (int) date.month, (int) date.day,
				(int) time.hour, (int) time.minute, (int) time.second
			);
		}
		else {
			return null;
		}
	}

	public string strftime (string format) {
		var time = to_local_gtime ();
		return (time != null) ? time.format (format) : "";
	}

	public string to_display_string () {
		return strftime ("%x %X");
	}

	public string to_string () {
		if (!is_valid ()) {
			return "";
		}
		if (!time.is_valid ()) {
			return date.to_string ();
		}
		return date.to_string () + " " + time.to_string ();
	}

	public int compare (Gth.DateTime other) {
		if (is_valid () && other.is_valid ()) {
			if (date.year < other.date.year) {
				return -1;
			}
			else if (date.year > other.date.year) {
				return 1;
			}
			else if (date.month < other.date.month) {
				return -1;
			}
			else if (date.month > other.date.month) {
				return 1;
			}
			else if (date.day < other.date.day) {
				return -1;
			}
			else if (date.day > other.date.day) {
				return 1;
			}
			else if (time.hour < other.time.hour) {
				return -1;
			}
			else if (time.hour > other.time.hour) {
				return 1;
			}
			else if (time.minute < other.time.minute) {
				return -1;
			}
			else if (time.minute > other.time.minute) {
				return 1;
			}
			else if (time.second < other.time.second) {
				return -1;
			}
			else if (time.second > other.time.second) {
				return 1;
			}
			else {
				return 0;
			}
		}
		else if (is_valid ()) {
			return -1;
		}
		else { // other is valid
			return 1;
		}
	}

	public static Gth.DateTime? get_from_exif_date (string? exif_date) {
		int year, month, day, hours, minutes, seconds;
		double useconds;
		if (!Lib.parse_exif_date (exif_date, out year, out month, out day,
			out hours, out minutes, out seconds, out useconds))
		{
			return null;
		}
		return new Gth.DateTime.from_ymd_hms (year, month, day, hours, minutes,
			seconds, (int) (useconds * 1000000));
	}
}

public struct Gth.Date {
	uint year;
	uint8 month;
	uint8 day;

	public Date () {
		year = INVALID_VALUE;
		month = INVALID_VALUE;
		day = INVALID_VALUE;
	}

	public Date.from_ymd (uint _year, uint8 _month, uint8 _day) {
		year = _year;
		month = _month;
		day = _day;
	}

	public Date.from_gdatetime (GLib.DateTime datetime) {
		int _year, _month, _day;
		datetime.get_ymd (out _year, out _month, out _day);
		if ((_year < 0) || (_month < 0) || (_day < 0)) {
			year = INVALID_VALUE;
			month = INVALID_VALUE;
			day = INVALID_VALUE;
		}
		else {
			year = (uint) _year;
			month = (uint8) _month;
			day = (uint8) _day;
		}
	}

	public Date.from_gdate (GLib.Date date) {
		if (!date.valid ()) {
			year = INVALID_VALUE;
			month = INVALID_VALUE;
			day = INVALID_VALUE;
		}
		else {
			year = (uint) date.get_year ();
			month = (uint8) date.get_month ();
			day = (uint8) date.get_day ();
		}
	}

	public Date.from_exif_date (string? exif_date) {
		var exif_datetime = DateTime.get_from_exif_date (exif_date);
		if (exif_datetime != null) {
			year = exif_datetime.date.year;
			month = exif_datetime.date.month;
			day = exif_datetime.date.day;
		}
		else {
			year = INVALID_VALUE;
			month = INVALID_VALUE;
			day = INVALID_VALUE;
		}
	}

	public Date.parse (string text) {
		var gdate = GLib.Date ();
		gdate.set_parse (text);
		this.from_gdate (gdate);
	}

	public bool is_valid () {
		return (year >= 1)
			&& (month >= 1)
			&& (month <= MAX_MONTH)
			&& (day >= 1)
			&& (day <= MAX_DAY);
	}

	public uint to_sort_order () {
		return is_valid () ? year * 1000 + month * 100 + day : 0;
	}

	public string to_string () {
		if (!is_valid ()) {
			return "0000-00-00";
		}
		return "%04d-%02d-%02d".printf ((int) year, (int) month, (int) day);
	}

	public string to_display_string () {
		if (!is_valid ()) {
			return "";
		}
		GLib.DateTime datetime = new GLib.DateTime (new GLib.TimeZone.local (), (int) year, (int) month, (int) day, 0, 0, 0);
		return datetime.format ("%x");
	}

	public GLib.DateTime? to_local_gdatetime () {
		if (is_valid ()) {
			return new GLib.DateTime (
				new TimeZone.local (),
				(int) year, (int) month, (int) day,
				0, 0, 0
			);
		}
		else {
			return null;
		}
	}

	public string to_exif_date () {
		if (is_valid ()) {
			return "%4u:%02u:%02u 00:00:00".printf (year, month, day);
		}
		else {
			return "";
		}
	}

	public int compare (Gth.Date other) {
		if (is_valid () && other.is_valid ()) {
			if (year < other.year) {
				return -1;
			}
			else if (year > other.year) {
				return 1;
			}
			else if (month < other.month) {
				return -1;
			}
			else if (month > other.month) {
				return 1;
			}
			else if (day < other.day) {
				return -1;
			}
			else if (day > other.day) {
				return 1;
			}
			else {
				return 0;
			}
		}
		else if (is_valid ()) {
			return -1;
		}
		else { // other is valid
			return 1;
		}
	}

	const uint8 MAX_MONTH = 12;
	const uint8 MAX_DAY = 31;
	const uint8 INVALID_VALUE = 0;
}

public struct Gth.Time {
	uint8 hour;
	uint8 minute;
	uint8 second;
	uint usecond;

	public Time () {
		hour = INVALID_HOUR;
		minute = 0;
		second = 0;
		usecond = 0;
	}

	public Time.from_hms (uint8 _hour, uint8 _min, uint8 _second, uint _usecond = 0) {
		hour = _hour;
		minute = _min;
		second = _second;
		usecond = _usecond;
	}

	public Time.from_gdatetime (GLib.DateTime datetime) {
		this.from_hms ((uint8) datetime.get_hour (), (uint8) datetime.get_minute (), (uint8) datetime.get_second ());
	}

	public Time.parse (string text) {
		hour = INVALID_HOUR;
		var tokens = text.split (":");
		if (tokens.length != 3) {
			tokens = text.split (".");
			if (tokens.length != 3) {
				return;
			}
		}
		int value;
		if (int.try_parse (tokens[0], out value, null, 10)) {
			if ((value >= 0) && (value <= MAX_HOUR)) {
				hour = (uint8) value;
				if (int.try_parse (tokens[1], out value, null, 10)) {
					if ((value >= 0) && (value <= MAX_MINUTE)) {
						minute = (uint8) value;
						if (int.try_parse (tokens[2], out value, null, 10)) {
							if ((value >= 0) && (value <= MAX_SECOND)) {
								// Time is valid.
								second = (uint8) value;
								usecond = 0;
							}
						}
					}
				}
			}
		}
	}

	public bool is_valid () {
		return (hour <= MAX_HOUR)
			&& (minute <= MAX_MINUTE)
			&& (second <= MAX_SECOND)
			&& (usecond <= MAX_MICROSECOND);
	}

	public string to_string () {
		if (!is_valid ()) {
			return "00:00:00";
		}
		return "%02d:%02d:%02d".printf (hour, minute, second);
	}

	public string to_display_string () {
		if (!is_valid ()) {
			return "";
		}
		if (second == 0) {
			return "%02d:%02d".printf (hour, minute);
		}
		return "%02d:%02d:%02d".printf (hour, minute, second);
	}

	public string to_exif_date () {
		if (is_valid ()) {
			return "%02u:%02u:%02u".printf (hour, minute, second);
		}
		else {
			return "";
		}
	}

	public int compare (Gth.Time other) {
		if (is_valid () && other.is_valid ()) {
			if (hour < other.hour) {
				return -1;
			}
			else if (hour > other.hour) {
				return 1;
			}
			else if (minute < other.minute) {
				return -1;
			}
			else if (minute > other.minute) {
				return 1;
			}
			else if (second < other.second) {
				return -1;
			}
			else if (second > other.second) {
				return 1;
			}
			else if (usecond < other.usecond) {
				return -1;
			}
			else if (usecond > other.usecond) {
				return 1;
			}
			else {
				return 0;
			}
		}
		else if (is_valid ()) {
			return -1;
		}
		else { // other is valid
			return 1;
		}
	}

	const uint8 MAX_HOUR = 23;
	const uint8 MAX_MINUTE = 59;
	const uint8 MAX_SECOND = 59;
	const uint MAX_MICROSECOND = 1000000 - 1;
	const uint8 INVALID_HOUR = MAX_HOUR + 1;
}
