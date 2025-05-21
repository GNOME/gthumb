public class Util {
	public static uint next_tick (owned Gth.IdleFunc function, int priority = Priority.DEFAULT) {
		return Timeout.add_full (priority, 0, () => {
			function ();
			return Source.REMOVE;
		});
	}

	public static uint after_next_rearrange (owned Gth.IdleFunc function) {
		return Timeout.add_full (Priority.LOW, 0, () => {
			function ();
			return Source.REMOVE;
		});
	}

	public static uint after_timeout (uint interval, owned Gth.IdleFunc function) {
		return Timeout.add (interval, () => {
			function ();
			return Source.REMOVE;
		});
	}

	public static uint after_seconds (uint seconds, owned Gth.IdleFunc function) {
		return Timeout.add_seconds (seconds, () => {
			function ();
			return Source.REMOVE;
		});
	}

	public static bool content_type_is_image (string content_type) {
		return ContentType.is_a (content_type, "image/*");
	}

	public static bool content_type_is_raw (string content_type) {
		return ContentType.is_a (content_type, "image/x-dcraw");
	}

	public static bool content_type_is_video (string content_type) {
		return ContentType.is_a (content_type, "video/*")
			|| (content_type == "application/ogg")
			|| (content_type == "application/x-matroska")
			|| (content_type == "application/vnd.ms-asf")
			|| (content_type == "application/vnd.rn-realmedia");
	}

	public static bool content_type_is_audio (string content_type) {
		return ContentType.is_a (content_type, "audio/*");
	}

	public static bool content_type_has_transparency (string content_type) {
		return (content_type == "image/png")
			|| (content_type == "image/gif")
			|| (content_type == "image/svg+xml")
			|| (content_type == "image/webp")
			|| (content_type == "image/jxl")
			|| (content_type == "image/avif")
			|| (content_type == "image/heif")
			|| (content_type == "image/heic")
			|| (content_type == "image/tiff")
			|| (content_type == "image/x-tga");
	}

	public static DateTime? get_time_from_exif_date (string? exif_date) {
		if (exif_date == null)
			return null;

		unowned char[] chars = (char[]) exif_date.data;
		int idx = 0;

		while (chars[idx].isspace ())
			idx++;

		if (chars[idx] == 0)
			return null;

		int parse_int (int expected_digits) {
			var digits = 0;
			var result = 0;
			while ((chars[idx] != 0) && (digits < expected_digits)) {
				if (!chars[idx].isdigit ()) {
					break;
				}
				digits++;
				result = (result * 10) + (chars[idx] - '0');
				idx++;
			}
			return (digits == expected_digits) ? result : -1;
		}

		var year = parse_int (4);
		if ((year < 1) || (year >= 10000))
			return null;

		if (chars[idx++] != ':')
			return null;

		var month = parse_int (2);
		if ((month < 1) || (month > 12))
			return null;

		if (chars[idx++] != ':')
			return null;

		var day = parse_int (2);
		if ((day < 1) || (day > 31))
			return null;

		if (chars[idx++] != ' ')
			return null;

		var hours = parse_int (2);
		if ((hours < 0) || (hours > 23))
			return null;

		if (chars[idx++] != ':')
			return null;

		var minutes = parse_int (2);
		if ((minutes < 0) || (minutes > 59))
			return null;

		if (chars[idx++] != ':')
			return null;

		var seconds = (double) parse_int (2);
		if ((seconds < 0) || (seconds > 59))
			return null;

		idx++;
		if ((chars[idx] == ',') || (chars[idx] == '.')) {
			idx++;
			var weight = 0.1;
			while ((weight > 0.0) && (chars[idx] != 0)) {
				if (!chars[idx].isdigit ()) {
					break;
				}
				seconds += weight * (chars[idx] - '0');
				weight /= 10.0;
				idx++;
			}
		}

		while (chars[idx].isspace ())
			idx++;

		if (chars[idx] != 0)
			return null;

		return new DateTime (new TimeZone.local (), year, month, day, hours, minutes, seconds);
	}

	public static int enum_index (string[] values, string? value, int default = 0) {
		if (value != null) {
			for (var i = 0; i < values.length; i++) {
				if (value == values[i]) {
					return i;
				}
			}
		}
		return default;
	}
}
