class Gth.Vintage : ImageOperation, ParametricOperation {
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
}
