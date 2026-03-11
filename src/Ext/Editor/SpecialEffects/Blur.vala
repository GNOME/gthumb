public class Gth.Blur : ImageOperation, ParametricOperation {
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

	public override Gth.Image? execute (Image input, Cancellable cancellable, bool for_preview = false) {
		if (input != null) {
			return input.blur (for_preview ? PREVIEW_RADIUS : (int) Math.round (amount * MAX_RADIUS), cancellable);
		}
		return null;
	}

	double amount = DEFAULT_AMOUNT;

	const double DEFAULT_AMOUNT = 0.4;
	const int MAX_RADIUS = 40;
	const int PREVIEW_RADIUS = 3;
}
