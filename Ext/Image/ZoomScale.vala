public class Gth.ZoomScale {
	public static float get_zoom (double adj_value) {
		var x = (adj_to_zoom (adj_value) - adj_to_zoom (ZOOM_ADJ_MIN)) / (adj_to_zoom (ZOOM_ADJ_MAX) - adj_to_zoom (ZOOM_ADJ_MIN));
		return (float) (ZOOM_MIN + (x * (ZOOM_MAX - ZOOM_MIN)));
	}

	public static double get_adj_value (float zoom) {
		var x = ((double) zoom - ZOOM_MIN) / (ZOOM_MAX - ZOOM_MIN);
		x = x * (adj_to_zoom (ZOOM_ADJ_MAX) - adj_to_zoom (ZOOM_ADJ_MIN)) + adj_to_zoom (ZOOM_ADJ_MIN);
		x = zoom_to_adj (x);
		return x.clamp (ZOOM_ADJ_MIN, ZOOM_ADJ_MAX);
	}

	inline static double adj_to_zoom (double x) {
		return Math.exp (x / 15 - Math.E);
	}

	inline static double zoom_to_adj (double z) {
		return (Math.log (z) + Math.E) * 15;
	}

	const float ZOOM_MIN = 0.05f;
	const float ZOOM_MAX = 10f;
	const float ZOOM_ADJ_MIN = 0f;
	const float ZOOM_ADJ_MAX = 100f;
}
