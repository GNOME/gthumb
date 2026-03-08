public class Gth.AdjustContrast : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/adjust-contrast.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);

		filter_grid = builder.get_object ("filter_grid") as Gth.FilterGrid;

		// Translators: filter that adjust the contrast balancing the white component.
		filter_grid.add (Method.STRETCH, new ContrastStretch (), _("Balanced"));

		// Translators: filter that adjust the color curves.
		filter_grid.add (Method.CURVE, new ContrastCurve (), _("Curves"));

		// Translators: filter that makes a linear transformation of the colors.
		filter_grid.add (Method.LINEAR, new ContrastLinear (), _("Linear"));

		// Translators: filter that equalizes the histogram.
		filter_grid.add (Method.EQUALIZE, new ContrastEqualize (), _("Equalize"));

		filter_grid.activated.connect ((id) => {
			var method = (Method) id;
			var operation = filter_grid.get_operation (method) as ParametricOperation;
			if (operation == null) {
				window.editor.set_action_bar (null);
			}
			else if (method == Method.STRETCH) {
				window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
				var scale = builder.get_object ("amount_scale") as Gtk.Scale;
				scale.digits = 0;
				scale.clear_marks ();
				scale.add_mark (ContrastStretch.amount_to_adj (0.005), Gtk.PositionType.BOTTOM, null);
				scale.add_mark (ContrastStretch.amount_to_adj (0.015), Gtk.PositionType.BOTTOM, null);
				scale.add_mark (ContrastStretch.amount_to_adj (0.025), Gtk.PositionType.BOTTOM, null);
				SignalHandler.block (amount_adjustment, amount_changed_id);
				amount_adjustment.configure (operation.get_amount (),
					ContrastStretch.amount_to_adj (0),
					ContrastStretch.amount_to_adj (0.030),
					1, 1, 0);
				SignalHandler.unblock (amount_adjustment, amount_changed_id);
			}
			else if (method == Method.LINEAR) {
				window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
				var scale = builder.get_object ("amount_scale") as Gtk.Scale;
				scale.digits = 0;
				scale.clear_marks ();
				SignalHandler.block (amount_adjustment, amount_changed_id);
				amount_adjustment.configure (operation.get_amount (), -50, 50, 1, 5, 0);
				SignalHandler.unblock (amount_adjustment, amount_changed_id);
			}
			else if (method == Method.EQUALIZE) {
				window.editor.set_action_bar (null);
				//window.editor.set_action_bar (builder.get_object ("equalize_action_bar") as Gtk.Widget);
			}
			else if (method == Method.CURVE) {
				window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
				var scale = builder.get_object ("amount_scale") as Gtk.Scale;
				scale.digits = 0;
				scale.clear_marks ();
				SignalHandler.block (amount_adjustment, amount_changed_id);
				amount_adjustment.configure (operation.get_amount (), -100, 100, 1, 1, 0);
				SignalHandler.unblock (amount_adjustment, amount_changed_id);
			}
			queue_update_preview ();
		});

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		add_default_controllers (image_view);
		image_view.image = viewer.image_view.image;
		image_view.set_first_state_from_view (viewer.image_view);

		window.editor.set_content (image_view);

		amount_adjustment = builder.get_object ("amount_adjustment") as Gtk.Adjustment;
		amount_changed_id = amount_adjustment.value_changed.connect (() => {
			var method = (Method) filter_grid.get_activated ();
			var operation = filter_grid.get_operation (method) as ParametricOperation;
			if ((operation != null) && operation.has_parameter ()) {
				operation.set_amount (amount_adjustment.value);
				queue_update_preview ();
			}
		});

		var linear_switch = builder.get_object ("equalize_linear") as Adw.SwitchRow;
		linear_switch.notify["active"].connect ((obj, param) => {
			var local_switch = obj as Adw.SwitchRow;
			var method = (Method) filter_grid.get_activated ();
			if (method == Method.EQUALIZE) {
				var operation = filter_grid.get_operation (method) as ContrastEqualize;
				if (operation != null) {
					operation.equalize_linear = local_switch.active;
					queue_update_preview ();
				}
			}
		});

		unowned var reset_button = builder.get_object ("reset_button") as Gtk.Button;
		reset_button.clicked.connect (() => reset_amount ());

		update_thumbnails ();
		filter_grid.activate_filter (Method.STRETCH);
	}

	public override void before_deactivate () {
		if (thumbnails_job != null) {
			thumbnails_job.cancel ();
		}
		builder = null;
	}

	public override ImageOperation? get_operation () {
		return filter_grid.get_active_operation ();
	}

	public override void update_options_from_operation (ImageOperation image_operation) {
		var operation = image_operation as ParametricOperation;
		if (operation.has_parameter ()) {
			SignalHandler.block (amount_adjustment, amount_changed_id);
			amount_adjustment.set_value (operation.get_amount ());
			SignalHandler.unblock (amount_adjustment, amount_changed_id);
		}
	}

	void reset_amount () {
		var id = filter_grid.get_activated ();
		var operation = filter_grid.get_operation (id) as ParametricOperation;
		if (operation != null) {
			operation.reset_amount ();
			update_preview ();
		}
	}

	void update_thumbnails () {
		if (thumbnails_job != null) {
			thumbnails_job.cancel ();
		}
		var job = window.new_job ("Update Thumbnails");
		thumbnails_job = job;
		try {
			var sample = viewer.image_view.image.resize (THUMBNAIL_SIZE,
				ResizeFlags.SQUARED, ScaleFilter.BOX, job.cancellable);
			filter_grid.update_previews (sample, job.cancellable);
		}
		catch (Error error) {
			window.show_error (error);
		}
		finally {
			job.done ();
			if (job == thumbnails_job) {
				thumbnails_job = null;
			}
		}
	}

	enum Method {
		NONE = 0,
		STRETCH,
		EQUALIZE,
		LINEAR,
		CURVE,
	}

	construct {
		title = _("Contrast");
		icon_name = "gth-adjust-contrast-symbolic";
	}

	Gtk.Builder builder;
	Gth.FilterGrid filter_grid;
	Job thumbnails_job = null;
	unowned Gtk.Adjustment amount_adjustment;
	ulong amount_changed_id = 0;

	const uint THUMBNAIL_SIZE = 140;
}


public class Gth.ContrastStretch : ImageOperation, ParametricOperation {
	public ContrastStretch () {
		amount = DEFAULT_AMOUNT;
	}

	public virtual bool has_parameter () {
		return true;
	}

	public virtual void set_amount (double _amount) {
		amount = _amount / 1000;
	}

	public virtual double get_amount () {
		return ContrastStretch.amount_to_adj (amount);
	}

	public static double amount_to_adj (double value) {
		return 1000.0 * value;
	}

	public virtual void reset_amount () {
		amount = DEFAULT_AMOUNT;
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable, bool for_preview = false) {
		if (input != null) {
			var output = input.dup ();
			if (output.stretch_histogram (amount, cancellable)) {
				return output;
			}
		}
		return null;
	}

	double amount;
	const double DEFAULT_AMOUNT = 0.005;
}


public class Gth.ContrastEqualize : ImageOperation, ParametricOperation {
	public bool equalize_linear;

	public ContrastEqualize () {
		equalize_linear = false;
	}

	public virtual bool has_parameter () {
		return false;
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable, bool for_preview = false) {
		if (input != null) {
			var output = input.dup ();
			if (output.equalize_histogram (equalize_linear, cancellable)) {
				return output;
			}
		}
		return null;
	}
}


public class Gth.ContrastLinear : ImageOperation, ParametricOperation {
	public ContrastLinear () {
		amount = DEFAULT_AMOUNT;
	}

	public virtual bool has_parameter () {
		return true;
	}

	public virtual void set_amount (double _amount) {
		amount = - _amount / 100.0;
	}

	public virtual double get_amount () {
		return - amount * 100.0;
	}

	public virtual void reset_amount () {
		amount = DEFAULT_AMOUNT;
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable, bool for_preview = false) {
		if (input != null) {
			var output = input.dup ();
			if (output.adjust_contrast (amount, cancellable)) {
				return output;
			}
		}
		return null;
	}

	double amount;
	const double DEFAULT_AMOUNT = -0.1;
}


public class Gth.ContrastCurve : ParametricCurveOperation, ParametricOperation {
	public ContrastCurve () {
		base (30);
	}

	public override void set_amount (double _amount) {
		amount = _amount / 100;
		update_points ();
	}

	public override double get_amount () {
		return amount * 100;
	}

	public override void update_points () {
		Point mid_point_1, mid_point_2;
		if (amount >= 0) {
			mid_point_1 = Point.interpolate (Point (62, 62), Point (75, 45), amount);
			mid_point_2 = Point.interpolate (Point (186, 186), Point (184, 213), amount);
		}
		else {
			mid_point_1 = Point.interpolate (Point (62, 62), Point (53, 91), - amount);
			mid_point_2 = Point.interpolate (Point (186, 186), Point (194, 172), - amount);
		}
		// stdout.printf ("> amount: %f -> p1: %s, p2: %s\n",
		// 	amount,
		// 	mid_point_1.to_string (),
		// 	mid_point_2.to_string ());
		points = new Points (
			{ Point (0, 0), mid_point_1, mid_point_2, Point (255, 255) },
			null,
			null,
			null
		);
	}
}
