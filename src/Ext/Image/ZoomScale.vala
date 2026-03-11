public class Gth.ZoomScale {
	public static float get_zoom (double adj_value, float zoom_min = ZOOM_MIN, float zoom_max = ZOOM_MAX) {
		var x = (adj_to_zoom (adj_value) - adj_to_zoom (ZOOM_ADJ_MIN)) / (adj_to_zoom (ZOOM_ADJ_MAX) - adj_to_zoom (ZOOM_ADJ_MIN));
		return (float) (zoom_min + (x * (zoom_max - zoom_min)));
	}

	public static double get_adj_value (float zoom, float zoom_min = ZOOM_MIN, float zoom_max = ZOOM_MAX) {
		var x = ((double) zoom - zoom_min) / (zoom_max - zoom_min);
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

	public const float ZOOM_MIN = 0.05f;
	public const float ZOOM_MAX = 10f;
	const float ZOOM_ADJ_MIN = 0f;
	const float ZOOM_ADJ_MAX = 100f;
}
