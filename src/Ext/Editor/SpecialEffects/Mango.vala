public class Gth.Mango : CurveOperation, ParametricOperation {
	public Mango () {
		points = new Points (
			null,
			{ Point (0, 0), Point (75, 67), Point (135, 155), Point (171, 193), Point (252, 255) },
			{ Point (0, 0), Point (84, 65), Point (162, 167), Point (255, 255) },
			{ Point (0, 0), Point (84, 74), Point (160, 150), Point (255, 255) }
		);
	}

	public override bool has_parameter () {
		return false;
	}
}
