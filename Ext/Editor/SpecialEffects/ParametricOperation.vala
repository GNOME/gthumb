public interface Gth.ParametricOperation : Object {
	public virtual bool has_parameter () {
		return false;
	}

	public virtual void set_amount (double _amount) {
		// void
	}

	public virtual double get_amount () {
		return 0.0;
	}

	public virtual void reset_amount () {
		// void
	}
}
