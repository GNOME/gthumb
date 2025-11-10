public class Gth.CurveOperation : ImageOperation {
	public Points points {
		set {
			_points = value;
			value_map = _points.to_value_map ();
		}
		get {
			return _points;
		}
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable) {
		if (input == null) {
			return null;
		}
		var output = input.dup ();
		output.apply_value_map (value_map);
		return output;
	}

	long [,] value_map;
	Points _points;
}
