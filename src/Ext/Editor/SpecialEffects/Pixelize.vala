public class Gth.Pixelize : ImageOperation, ParametricOperation {
	public Pixelize (double _amount = DEFAULT_AMOUNT) {
		amount = _amount.clamp (0, MAX_AMOUNT);
	}

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

	public override Gth.Image? execute (Image input, Cancellable cancellable, bool for_preview = false) {
		if (input != null) {
			var output = input.dup ();
			if (output.pixelize (for_preview ? PREVIEW_AMOUNT : ((uint) amount), cancellable)) {
				return output;
			}
		}
		return null;
	}

	double amount = DEFAULT_AMOUNT;

	public const double DEFAULT_AMOUNT = 5;
	const double MAX_AMOUNT = 50;
	const uint PREVIEW_AMOUNT = 8;
}
