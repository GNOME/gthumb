using GLib;

namespace Gth {
	[CCode (cheader_filename = "lib/gth-point.h", has_type_id = false)]
	public struct Point {
		public double x;
		public double y;
		public Point (double x, double y);
		public Point.interpolate (Point p1, Point p2, double alpha);
		public double distance (Point other);
		public string to_string ();
	}
}
