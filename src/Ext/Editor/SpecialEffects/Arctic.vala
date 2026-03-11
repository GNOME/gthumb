public class Gth.Arctic : CurveOperation, ParametricOperation {
	public Arctic () {
		points = new Points (
			null,
			{ Point (0, 0), Point (133, 122), Point (255, 255) },
			{ Point (0, 0), Point (64, 57), Point (176, 186), Point (255, 255) },
			{ Point (0, 0), Point (180, 207), Point (255, 255) }
		);
	}

	public override bool has_parameter () {
		return false;
	}
}
