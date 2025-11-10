public struct Gth.Points {
	Point[] value;
	Point[] red;
	Point[] green;
	Point[] blue;

	public long[,] to_value_map () {
		var value_map = new long[4, 256];
		for (var channel = Channel.VALUE; channel <= Channel.BLUE; channel += 1) {
			var curve = new Bezier (get_points (channel));
			for (var v = 0; v <= 255; v++) {
				var u = curve.eval (v);
				if (channel != Channel.VALUE) {
					u = value_map[Channel.VALUE, (int) u];
				}
				value_map[channel, v] = (long) u;
			}
		}
		return value_map;
	}

	unowned Point[] get_points (Channel channel) {
		if (channel == Channel.VALUE) {
			return value;
		}
		else if (channel == Channel.RED) {
			return red;
		}
		else if (channel == Channel.GREEN) {
			return green;
		}
		else if (channel == Channel.BLUE) {
			return blue;
		}
		return null;
	}
}
