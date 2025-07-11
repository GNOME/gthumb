using GLib;

[CCode (cheader_filename = "lib/util.h")]
namespace Lib {
	[CCode (cname = "_g_format_double")]
	public static string format_double (double value, int max_decimal_digits);
}
