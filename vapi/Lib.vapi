using GLib;

[CCode (cheader_filename = "lib/util.h")]
namespace Lib {
	[CCode (cname = "_g_format_double")]
	public static string format_double (double value, int max_decimal_digits);

	[CCode (cname = "_g_format_duration_for_display")]
	public static string format_duration_for_display (int64 msecs, out int hours = null, out int minutes = null);

	[CCode (cname = "_g_format_duration_not_localized")]
	public static string format_duration_not_localized (int64 msecs);
}
