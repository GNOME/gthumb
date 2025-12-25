public class Gth.Dither : ImageOperation, ParametricOperation {
	public enum Method {
		ORDERED,
		DOTS;

		public unowned string to_state () {
			return STATE[this];
		}

		public static Method from_state (string? state) {
			var idx = Util.enum_index (STATE, state);
			return (Method) idx;
		}

		const string[] STATE = { "ordered", "dots" };
	}

	public Method method = Method.ORDERED;

	public override Gth.Image? execute (Image input, Cancellable cancellable) {
		if (input != null) {
			var output = input.dup ();
			if ((method == Method.ORDERED) && output.dither_ordered (cancellable)) {
				return output;
			}
			if ((method == Method.DOTS) && output.dither_error_diffusion (cancellable)) {
				return output;
			}
		}
		return null;
	}
}
