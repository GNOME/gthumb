using GLib;

namespace Gth {
	[Compact]
	[CCode (cheader_filename = "lib/gth-points.h",
		ref_function = "gth_points_ref",
		unref_function = "gth_points_unref",
		has_type_id = false)]
	public class Points {
		[CCode (array_length_cname = "value_size")]
		public Point[] value;

		[CCode (array_length_cname = "red_size")]
		public Point[] red;

		[CCode (array_length_cname = "green_size")]
		public Point[] green;

		[CCode (array_length_cname = "blue_size")]
		public Point[] blue;

		public Points (owned Point[] value, owned Point[] red, owned Point[] green, owned Point[] blue);
		public Point[] get_channel (Gth.Channel channel);
		public uint8[] get_value_map ();
	}
}
