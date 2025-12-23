class Gth.Vintage : ImageOperation, ParametricOperation {
	public override bool has_parameter () {
		return false;
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
		var output = input.dup ();
		if (output.grayscale (0.3333, 0.3333, 0.3333, 1, cancellable)
			&& output.colorize (0.55, 0.3, 0, cancellable)
			&& output.adjust_contrast (0.1, cancellable)
			&& output.apply_vignette (-0.8, cancellable)
		)
		{
			return output;
		}
		return null;
	}

	double amount = 0.5;
}
