public class Gth.Lomo : ImageOperation, ParametricOperation {
	public override Gth.Image? execute (Image input, Cancellable cancellable, bool for_preview = false) {
		if (input != null) {
			var output = input.dup ();
			var points = new Points (
				null,
				{ Point (0, 0), Point (56, 45), Point (195, 197), Point (255, 216) },
				{ Point (0, 0), Point (65, 40), Point (162, 174), Point (238, 255) },
				{ Point (0, 0), Point (68, 79), Point (210, 174), Point (255, 255) }
			);
			if (output.apply_curve (points, cancellable)) {
				if (output.progressive_blur (6, cancellable)) {
					if (output.soft_light_with_radial_gradient (cancellable)) {
						return output;
					}
				}
			}
		}
		return null;
	}
}
