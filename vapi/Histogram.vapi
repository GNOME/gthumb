using GLib;

namespace Gth {
	[CCode (cheader_filename = "lib/gth-histogram.h")]
	public enum Channel {
		VALUE,
		RED,
		GREEN,
		BLUE,
		ALPHA;
		[CCode (cname= "GTH_HISTOGRAM_N_CHANNELS")]
		public const int SIZE;
	}

	[CCode (cheader_filename = "lib/gth-histogram.h", type_id = "gth_histogram_get_type ()")]
	public class Histogram : Object {
		public signal void changed ();
		public Histogram ();
		public void calculate (Image image);
		public double get_count (int start, int end);
		public double get_value (Channel channel, int bin);
		public double get_channel_value (Channel channel, int bin);
		public double get_channel_max (Channel channel);
		public double get_max (Channel channel);
		public uchar get_min_value (Channel channel);
		public uchar get_max_value (Channel channel);
		public int get_n_pixels ();
		public int get_n_channels ();
	}
}
