using GLib;

namespace Gth {
	[CCode (cheader_filename = "lib/gth-point.h", has_type_id = false)]
	public struct Point {
		double x;
		double y;
		public double distance (Point other);
	}

	[CCode (cheader_filename = "lib/gth-curve.h")]
	public class Curve : Object {
		[CCode (array_length_cname = "n", array_length_type = "int")]
		Point[] p {
			[CCode (cname = "gth_curve_set_point")] set;
			get;
		}
		public void setup ();
		public double eval (double x);
	}

	[CCode (cheader_filename = "lib/gth-curve.h")]
	public class Spline : Curve {
		public Spline (Point[] points);
	}

	[CCode (cheader_filename = "lib/gth-curve.h")]
	public class CSpline : Curve {
		public CSpline (Point[] points);
	}

	[CCode (cheader_filename = "lib/gth-curve.h")]
	public class Bezier : Curve {
		public Bezier (Point[] points);
	}
}
