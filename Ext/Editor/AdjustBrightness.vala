public class Gth.AdjustBrightness : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/adjust-brightness.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);

		filter_grid = builder.get_object ("filter_grid") as Gth.FilterGrid;

		// Translators: filter that executes a gamma correction.
		filter_grid.add (Method.GAMMA, new GammaCorrection (), _("Gamma"));

		// Translators: filter that adjust the color curves.
		filter_grid.add (Method.CURVE, new BrightnessCurve (), _("Curves"));

		// Translators: filter that makes a linear tranformation of the colors.
		filter_grid.add (Method.LINEAR, new BrightnessLinear (), _("Linear"));

		filter_grid.activated.connect ((id) => {
			var method = (Method) id;
			var operation = filter_grid.get_operation (method) as ParametricOperation;
			if (operation == null) {
				window.editor.set_action_bar (null);
			}
			else if (method == Method.GAMMA) {
				window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
				var scale = builder.get_object ("amount_scale") as Gtk.Scale;
				scale.digits = 2;
				SignalHandler.block (amount_adjustment, amount_changed_id);
				amount_adjustment.configure (operation.get_amount (), 0, 2, 0.1, 0.1, 0);
				SignalHandler.unblock (amount_adjustment, amount_changed_id);
			}
			else if (method == Method.LINEAR) {
				window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
				var scale = builder.get_object ("amount_scale") as Gtk.Scale;
				scale.digits = 0;
				SignalHandler.block (amount_adjustment, amount_changed_id);
				amount_adjustment.configure (operation.get_amount (), -50, 50, 1, 5, 0);
				SignalHandler.unblock (amount_adjustment, amount_changed_id);
			}
			else if (method == Method.CURVE) {
				window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
				var scale = builder.get_object ("amount_scale") as Gtk.Scale;
				scale.digits = 0;
				SignalHandler.block (amount_adjustment, amount_changed_id);
				amount_adjustment.configure (operation.get_amount (), -100, 100, 1, 5, 0);
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
		amount_changed_id = amount_adjustment.value_changed.connect ((local_adj) => {
			var method = (Method) filter_grid.get_activated ();
			var operation = filter_grid.get_operation (method) as ParametricOperation;
			if ((operation != null) && operation.has_parameter ()) {
				operation.set_amount (amount_adjustment.value);
				queue_update_preview ();
			}
		});

		unowned var reset_button = builder.get_object ("reset_button") as Gtk.Button;
		reset_button.clicked.connect (() => reset_amount ());

		update_thumbnails ();
		filter_grid.activate (Method.GAMMA);
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
		GAMMA,
		LINEAR,
		CURVE;
	}

	construct {
		title = _("Brightness");
		icon_name = "gth-adjust-brightness-symbolic";
	}

	Gtk.Builder builder;
	Gth.FilterGrid filter_grid;
	Job thumbnails_job = null;
	unowned Gtk.Adjustment amount_adjustment;
	ulong amount_changed_id = 0;

	const uint THUMBNAIL_SIZE = 140;
}


public class Gth.BrightnessCurve : ParametricCurveOperation, ParametricOperation {
	public BrightnessCurve () {
		base (20);
	}

	public override void set_amount (double _amount) {
		amount = _amount / 100.0;
		update_points ();
	}

	public override double get_amount () {
		return amount * 100;
	}

	public override void update_points () {
		Point start_point, end_point;
		if (amount >= 0) {
			start_point = Point.interpolate (Point (0, 0), Point (95, 0), amount);
			end_point = Point.interpolate (Point (255, 255), Point (255, 160), amount);
		}
		else {
			start_point = Point.interpolate (Point (0, 0), Point (0, 65), - amount);
			end_point = Point.interpolate (Point (255, 255), Point (190, 255), - amount);
		}
		points = new Points (
			{ start_point, end_point },
			null,
			null,
			null
		);
	}
}


public class Gth.GammaCorrection : ImageOperation, ParametricOperation {
	public GammaCorrection () {
		reset_amount ();
	}

	public override bool has_parameter () {
		return true;
	}

	public override void set_amount (double _amount) {
		amount = _amount;
	}

	public override double get_amount () {
		return amount;
	}

	public override void reset_amount () {
		amount = 1.2;
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable) {
		if (input != null) {
			var output = input.dup ();
			if (output.gamma_correction (amount, cancellable)) {
				return output;
			}
		}
		return null;
	}

	double amount;
}


public class Gth.BrightnessLinear : ImageOperation, ParametricOperation {
	public BrightnessLinear () {
		reset_amount ();
	}

	public override bool has_parameter () {
		return true;
	}

	public override void set_amount (double _amount) {
		amount = _amount / 100.0;
	}

	public override double get_amount () {
		return amount * 100.0;
	}

	public override void reset_amount () {
		amount = 0.2;
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable) {
		if (input != null) {
			var output = input.dup ();
			if (output.adjust_brightness (amount, cancellable)) {
				return output;
			}
		}
		return null;
	}

	double amount;
}
