public class Gth.CurveOperation : ImageOperation {
	public Points points;

	public override Gth.Image? execute (Image input, Cancellable cancellable) {
		if (input != null) {
			var output = input.dup ();
			if (output.apply_curve (points, cancellable)) {
				return output;
			}
		}
		return null;
	}
}
