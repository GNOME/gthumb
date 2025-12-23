class Gth.ParametricCurveOperation : CurveOperation, ParametricOperation {
	public double default_amount;
	public double amount;

	public ParametricCurveOperation (double _default_amount = 0.0) {
		default_amount = _default_amount;
		set_amount (_default_amount);
	}

	public override bool has_parameter () {
		return true;
	}

	public override void set_amount (double _amount) {
		amount = _amount;
		update_points ();
	}

	public override double get_amount () {
		return amount;
	}

	public override void reset_amount () {
		set_amount (default_amount);
	}

	public virtual void update_points () {
		// void
	}
}
