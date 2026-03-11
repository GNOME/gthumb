public class Gth.SolidColor : ImageOperation, ParametricOperation {
	public Gdk.RGBA color;

	public override Gth.Image? execute (Image input, Cancellable cancellable, bool for_preview = false) {
		if (input == null) {
			return null;
		}
		var result = new Image (input.width, input.height);
		result.fill_color (color);
		return result;
	}

	construct {
		color = { 0, 0, 0, 1 };
	}
}
