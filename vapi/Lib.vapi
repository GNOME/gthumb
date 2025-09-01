using GLib;

[CCode (cheader_filename = "lib/util.h")]
namespace Lib {
	[CCode (cname = "_g_format_double")]
	public static string format_double (double value, int max_decimal_digits);

	[CCode (cname = "_g_format_duration_for_display")]
	public static string format_duration_for_display (int64 msecs, out int hours = null, out int minutes = null);

	[CCode (cname = "_g_format_duration_not_localized")]
	public static string format_duration_not_localized (int64 msecs);

	[CCode (cname = "scale_keeping_ratio")]
	public static bool scale_keeping_ratio (ref uint width, ref uint height, uint max_width, uint max_height, bool allow_upscaling = false);

	[CCode (cname = "_g_parse_exif_date")]
	public static bool parse_exif_date (string *exif_date,
		out int out_year, out int out_month, out int out_day,
		out int out_hour, out int out_minute, out int out_second,
		out double out_usecond);
}
