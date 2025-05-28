public class Gth.DateTime {
	public Gth.Date date;
	public Gth.Time time;

	public DateTime () {
		clear ();
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
		else if (date_is_valid ()) {
			return "%4u:%02u:%02u 00:00:00".printf (
				date.year, date.month, date.day
			);
		}
		else {
			return "";
		}
	}

	public bool set_from_exif_date (string? exif_date) {
		var time = Util.get_time_from_exif_date (exif_date);
		if (time != null) {
			set_from_gtime (time);
			return true;
		}
		else {
			clear ();
			return false;
		}
	}

	public void set_from_gtime (GLib.DateTime gtime) {
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

	public string to_string () {
		return strftime ("%x %X");
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

	public bool is_valid () {
		return (year >= 1)
			&& (month >= 1)
			&& (month <= MAX_MONTH)
			&& (day >= 1)
			&& (day <= MAX_DAY);
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

	public bool is_valid () {
		return (hour <= MAX_HOUR)
			&& (minute <= MAX_MINUTE)
			&& (second <= MAX_SECOND)
			&& (usecond <= MAX_MICROSECOND);
	}

	const uint8 MAX_HOUR = 23;
	const uint8 MAX_MINUTE = 59;
	const uint8 MAX_SECOND = 59;
	const uint MAX_MICROSECOND = 1000000 - 1;
	const uint8 INVALID_HOUR = MAX_HOUR + 1;
}
