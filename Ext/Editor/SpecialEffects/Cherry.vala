public class Gth.Cherry : CurveOperation, ParametricOperation {
	public Cherry () {
		points = new Points (
			null,
			{ Point (0, 12), Point (74, 79), Point (134, 156), Point (188, 209), Point (239, 255) },
			{ Point (12, 0), Point (78, 67), Point (138, 140), Point (189, 189), Point (252, 233) },
			{ Point (0, 8), Point (77, 100), Point (139, 140), Point (202, 186), Point (255, 244) }
		);
	}

	public override bool has_parameter () {
		return false;
	}
}
