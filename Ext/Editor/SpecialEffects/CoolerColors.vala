class Gth.CoolerColors : ParametricCurveOperation {
	public CoolerColors () {
		base (0.158730159);
	}

	public override void update_points () {
		var red_point = Point.interpolate (Point (127, 127), Point (183, 74), amount);
		var blue_point = Point.interpolate (Point (127, 127), Point (77, 169), amount);
		points = new Points (
			null,
			{ Point (0, 0), red_point /*Point (136, 119)*/, Point (255, 255) },
			null,
			{ Point (0, 0), blue_point /*Point (117, 136)*/, Point (255, 255) }
		);
	}
}
