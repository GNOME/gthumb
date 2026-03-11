public abstract class Gth.Curve {
	public abstract double eval (double x);

	public virtual void setup () {
		p = points.data;
		n = points.length;
	}

	public GenericArray<Point?> points;
	public Point?[] p;
	public int n;
}

// http://en.wikipedia.org/wiki/Spline_interpolation#Algorithm_to_find_the_interpolating_cubic_spline
public class Gth.Spline : Gth.Curve {
	public double[] k;
	public bool singular;

	public override void setup () {
		base.setup ();
		var matrix = new double[n + 1, n + 2];
		matrix[0, 0  ] = 2.0 / (p[1].x - p[0].x);
		matrix[0, 1  ] = 1.0 / (p[1].x - p[0].x);
		matrix[0, n+1] = 3.0 * (p[1].y - p[0].y) / ((p[1].x - p[0].x) * (p[1].x - p[0].x));
		for (var i = 1; i < n; i++) {
			matrix[i, i-1] = 1.0 / (p[i].x - p[i-1].x);
			matrix[i, i  ] = 2.0 * (1.0 / (p[i].x - p[i-1].x) + 1.0 / (p[i+1].x - p[i].x));
			matrix[i, i+1] = 1.0 / (p[i+1].x - p[i].x);
			matrix[i, n+1] = 3.0 * ( (p[i].y - p[i-1].y) / ((p[i].x - p[i-1].x) * (p[i].x - p[i-1].x)) + (p[i+1].y - p[i].y) / ((p[i+1].x - p[i].x) * (p[i+1].x - p[i].x)) );
		}
		matrix[n, n-1] = 1.0 / (p[n].x - p[n-1].x);
		matrix[n, n  ] = 2.0 / (p[n].x - p[n-1].x);
		matrix[n, n+1] = 3.0 * (p[n].y - p[n-1].y) / ((p[n].x - p[n-1].x) * (p[n].x - p[n-1].x));

		k = new double[n];
		for (var i = 0; i < n + 1; i++) {
			k[i] = 1.0;
		}
		singular = Lib.gauss_jordan_solver (matrix, k);
	}

	public override double eval (double x) {
		if (singular) {
			return x;
		}
		var i = 1;
		while (p[i].x < x) {
			i++;
		}
		var t = (x - p[i-1].x) / (p[i].x - p[i-1].x);
		var a = k[i-1] * (p[i].x - p[i-1].x) - (p[i].y - p[i-1].y);
		var b = - k[i] * (p[i].x - p[i-1].x) + (p[i].y - p[i-1].y);
		var y = Math.round ( ((1-t) * p[i-1].y) + (t * p[i].y) + (t * (1-t) * (a * (1-t) + b * t)) );
		return y.clamp (0, 255);
	}
}

// https://en.wikipedia.org/wiki/Cubic_Hermite_spline
public class Gth.CSpline : Gth.Curve {
	public double[] tangents;

	public override void setup () {
		base.setup ();
		tangents = new double[n];
		//finite_difference (tangents);
		catmull_rom_spline (tangents);
		//monotonic_spline (tangents);
	}

	void finite_difference (double[] t) {
		for (var k = 0; k < n; k++) {
			t[k] = 0;
			if (k > 0) {
				t[k] += (p[k].y - p[k-1].y) / (2 * (p[k].x - p[k-1].x));
			}
			if (k < n - 1) {
				t[k] += (p[k+1].y - p[k].y) / (2 * (p[k+1].x - p[k].x));
			}
		}
	}

	void catmull_rom_spline (double[] t) {
		for (var k = 0; k < n; k++) {
			t[k] = 0;
			if (k == 0) {
				t[k] = (p[k+1].y - p[k].y) / (p[k+1].x - p[k].x);
			}
			else if (k == n - 1) {
				t[k] = (p[k].y - p[k-1].y) / (p[k].x - p[k-1].x);
			}
			else {
				t[k] = (p[k+1].y - p[k-1].y) / (p[k+1].x - p[k-1].x);
			}
		}
	}

	// Monotonic spline (Fritsch–Carlson method) (https://en.wikipedia.org/wiki/Monotone_cubic_interpolation)
	void monotonic_spline (double[] t) {
		// TODO
	}

	public override double eval (double x) {
		return 0;
	}
}

public class Gth.Bezier : Gth.Curve {
	public double[,] k;
	public bool linear;

	public override void setup () {
		base.setup ();

		linear = (n <= 1);
		if (linear) {
			return;
		}

		k = new double[n - 1, 4]; // 4 points for each interval
		for (var i = 0; i < n - 1; i++) {
			Point? a = (i == 0) ? null: p[i - 1];
			Point? b = (i == n - 2) ? null : p[i + 2];
			setup_control_point (a, p[i], p[i+1], b, i);
		}
	}

	public override double eval (double x) {
		if (linear) {
			return x;
		}
		var i = 1;
		while (p[i].x < x) {
			i++;
		}
		i--;
		var t = (x - p[i].x) / (p[i+1].x - p[i].x);
		var t2 = t * t;
		var s = 1.0 - t;
		var s2 = s * s;
		var y = Math.round (s2*s*k[i,0] + 3*s2*t*k[i,1] + 3*s*t2*k[i,2] + t2*t*k[i,3]);
		return y.clamp (0, 255);
	}

	void setup_control_point (Point? a, Point p1, Point p2, Point? b, int i) {
		double c1_y, c2_y;
		if ((a != null) && (b != null)) {
			var slope = (p2.y - a.y) / (p2.x - a.x);
			c1_y = p1.y + slope * (p2.x - p1.x) / 3.0;

			slope = (b.y - p1.y) / (b.x - p1.x);
			c2_y = p2.y - slope * (p2.x - p1.x) / 3.0;
		}
		else if ((a == null) && (b != null)) {
			var slope = (b.y - p1.y) / (b.x - p1.x);
			c2_y = p2.y - slope * (p2.x - p1.x) / 3.0;
			c1_y = p1.y + (c2_y - p1.y) / 2.0;
		}
		else if ((a != null) && (b == null)) {
			var slope = (p2.y - a.y) / (p2.x - a.x);
			c1_y = p1.y + slope * (p2.x - p1.x) / 3.0;
			c2_y = p2.y + (c1_y - p2.y) / 2.0;
		}
		else { // (a == null) && (b == null)
			c1_y = p1.y + (p2.y - p1.y) / 3.0;
			c2_y = p1.y + (p2.y - p1.y) * 2.0 / 3.0;
		}
		k[i,0] = p1.y;
		k[i,1] = c1_y;
		k[i,2] = c2_y;
		k[i,3] = p2.y;
	}
}

public struct Gth.Point {
	double x;
	double y;

	public double distance (Point other) {
		var dx = (this.x - other.x);
		var dy = (this.y - other.y);
		return Math.sqrt ((dx * dx) + (dy * dy));
	}
}
