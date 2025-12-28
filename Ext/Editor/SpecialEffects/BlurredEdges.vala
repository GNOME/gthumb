public class Gth.BlurredEdges : ImageOperation, ParametricOperation {
	public override bool has_parameter () {
		return true;
	}

	public override void set_amount (double _amount) {
		amount = _amount;
	}

	public override double get_amount () {
		return amount;
	}

	public override void reset_amount () {
		set_amount (DEFAULT_AMOUNT);
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable) {
		if (input != null) {
			var output = input.dup ();
			if (output.progressive_blur ((int) Math.round (amount * 20), cancellable)) {
				return output;
			}
		}
		return null;
	}

	double amount = DEFAULT_AMOUNT;

	const double DEFAULT_AMOUNT = 0.5;
}
