public class Gth.Pixelize : ImageOperation, ParametricOperation {
	public override bool has_parameter () {
		return true;
	}

	public override void set_amount (double _amount) {
		amount = _amount * MAX_AMOUNT;
	}

	public override double get_amount () {
		return amount / MAX_AMOUNT;
	}

	public override void reset_amount () {
		set_amount (DEFAULT_AMOUNT);
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable) {
		if (input != null) {
			var output = input.dup ();
			if (output.pixelize ((uint) amount, cancellable)) {
				return output;
			}
		}
		return null;
	}

	double amount = DEFAULT_AMOUNT;

	const double DEFAULT_AMOUNT = 20;
	const double MAX_AMOUNT = 50;
}
