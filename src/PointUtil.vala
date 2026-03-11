public class Gth.PointUtil {
	const double DRAG_THRESHOLD = 1;

	public static bool drag_threshold (Graphene.Point point, double px, double py) {
		var dx = (point.x - px).abs ();
		var dy = (point.y - py).abs ();
		return (dx >= DRAG_THRESHOLD) || (dy >= DRAG_THRESHOLD);
	}

	public static Graphene.Point point_from_click (double x, double y) {
		return Graphene.Point ().init ((float) x, (float) y);
	}

	public static float get_angle (Graphene.Point p1, Graphene.Point p2) {
		var angle = 0.0;
		var dx = p2.x - p1.x;
		var dy = p2.y - p1.y;
		if (dx >= 0) {
			if (dy >= 0) {
				angle = Math.atan2 (dy, dx);
			}
			else {
				angle = Math.PI * 2 - Math.atan2 (-dy, dx);
			}
		}
		else {
			if (dy >= 0) {
				angle = Math.PI - Math.atan2 (dy, -dx);
			}
			else {
				angle = Math.PI + Math.atan2 (-dy, -dx);
			}
		}
		return (float) angle;
	}
}
