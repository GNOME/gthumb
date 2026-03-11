public class Gth.Desert : CurveOperation, ParametricOperation {
	public Desert () {
		points = new Points (
			{ Point (0, 0), Point (115, 145), Point (255, 255) },
			{ Point (0, 0), Point (71, 66), Point (208, 204), Point (255, 255) },
			{ Point (0, 0), Point (71, 55), Point (200, 206), Point (255, 255) },
			{ Point (0, 0), Point (232, 185), Point (255, 248) }
		);
	}

	public override bool has_parameter () {
		return false;
	}
}
