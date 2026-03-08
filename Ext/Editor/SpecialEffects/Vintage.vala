public class Gth.Vintage : ImageOperation, ParametricOperation {
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
		amount = DEFAULT_AMOUNT;
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable, bool for_preview = false) {
		if (input == null) {
			return null;
		}

		var output = input.dup ();
		if (output.grayscale (0.3333, 0.3333, 0.3333, 1, cancellable)
			&& output.colorize (Util.interpolate (0.45, 0.65, amount), Util.interpolate (0.2, 0.4, amount), 0, cancellable)
			&& output.adjust_contrast (Util.interpolate (-0.1, 0.3, amount), cancellable)
			&& output.apply_vignette (Util.interpolate (-0.4, -1.2, amount), cancellable))
		{
			return output;
		}
		return null;
	}

	double amount = DEFAULT_AMOUNT;
	const double DEFAULT_AMOUNT = 0.5;
}
