public class Gth.NegativeColors : ImageOperation, ParametricOperation {
	public override Gth.Image? execute (Image input, Cancellable cancellable, bool for_preview = false) {
		if (input == null) {
			return null;
		}
		var result = input.dup ();
		result.negative_colors ();
		return result;
	}
}
