class Gth.BlurredEdges : ImageOperation, ParametricOperation {
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
		set_amount (0.5);
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable) {
		if (input == null) {
			return null;
		}
		var blurred = input.dup ();
		if (!blurred.blur ((int) Math.round (amount * 10), cancellable)) {
			return null;
		}
		var output = input.dup ();
		if (!output.apply_radial_mask (blurred, amount, cancellable)) {
			return null;
		}
		return output;
	}

	double amount = 0.5;
}
