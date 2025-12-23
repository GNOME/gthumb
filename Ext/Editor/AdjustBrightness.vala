public class Gth.AdjustBrightness : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/adjust-brightness.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);
		window.editor.sidebar.insert_action_group ("brightness", action_group);

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.resized.connect (() => update_preview_on_resize ());
		add_default_controllers (image_view);

		window.editor.set_content (image_view);

		amount_adjustment = builder.get_object ("amount_adjustment") as Gtk.Adjustment;
		amount_changed_id = amount_adjustment.value_changed.connect ((local_adj) => {
			if (method == Method.CURVE) {
				var operation = operations[method] as BrightnessCurve;
				operation.amount = amount_adjustment.value;
				queue_update_preview ();
			}
			else {
				var operation = operations[method] as Operation;
				operation.amount = amount_adjustment.value;
				queue_update_preview ();
			}
		});

		amount_label = builder.get_object ("amount_label") as Gtk.Label;
		amount_adjustment.value_changed.connect ((local_adj) => {
			if (method == Method.CURVE) {
				var int_value = (int) Math.round (local_adj.value);
				amount_label.label = "%d".printf (int_value);
			}
			else {
				amount_label.label = "%.2f".printf (local_adj.value);
			}
		});

		unowned var reset_button = builder.get_object ("reset_button") as Gtk.Button;
		reset_button.clicked.connect (() => reset_amount ());

		var preview_switch = builder.get_object ("preview") as Adw.SwitchRow;
		preview_switch.notify["active"].connect ((obj, param) => {
			var local_switch = obj as Adw.SwitchRow;
			show_preview (local_switch.active);
		});

		operations = new ImageOperation[3];
		operations[Method.GAMMA] = new Operation (Method.GAMMA, 1.0);
		operations[Method.LINEAR] = new Operation (Method.LINEAR, 0.0);
		operations[Method.CURVE] = new BrightnessCurve ();
		set_method (Method.GAMMA);
	}

	public override void before_deactivate () {
		window.editor.sidebar.insert_action_group ("brightness", null);
		builder = null;
	}

	public override ImageOperation? get_operation () {
		return operations[method];
	}

	public override void update_options_from_operation (ImageOperation image_operation) {
		if (image_operation is BrightnessCurve) {
			var operation = image_operation as BrightnessCurve;
			SignalHandler.block (amount_adjustment, amount_changed_id);
			amount_adjustment.set_value (operation.amount);
			SignalHandler.unblock (amount_adjustment, amount_changed_id);
		}
		else {
			var operation = image_operation as Operation;
			SignalHandler.block (amount_adjustment, amount_changed_id);
			amount_adjustment.set_value (operation.amount);
			SignalHandler.unblock (amount_adjustment, amount_changed_id);
		}
	}

	void reset_amount () {
		if (method == Method.CURVE) {
			var operation = operations[method] as BrightnessCurve;
			operation.amount = 0.0;
			update_preview ();
		}
		else {
			var operation = operations[method] as Operation;
			operation.amount = operation.default_amount;
			update_preview ();
		}
	}

	enum Method {
		GAMMA = 0,
		LINEAR,
		CURVE;
	}

	class BrightnessCurve : CurveOperation {
		public double amount {
			get { return - _amount * 100.0; }
			set {
				_amount = - value / 100.0;
				update_points ();
			}
		}

		void update_points () {
			var value_point = Point.interpolate (Point (127, 127), Point (77, 169), _amount);
			points = Points () {
				value = { Point (0, 0), value_point, Point (255, 255) },
				red   = null,
				green = null,
				blue  = null,
			};
		}

		double _amount;
	}

	class Operation : ImageOperation {
		public Method method;
		public double amount;
		public double default_amount;

		public Operation (Method _method, double _amount = 0.0) {
			method = _method;
			amount = _amount;
			default_amount = _amount;
		}

		public override Gth.Image? execute (Image input, Cancellable cancellable) {
			if (input == null) {
				return null;
			}
			var output = input.dup ();
			var completed = false;
			switch (method) {
			case Method.GAMMA:
				completed = output.gamma_correction (amount, cancellable);
				break;
			case Method.LINEAR:
				completed = output.adjust_brightness (amount, cancellable);
				break;
			default:
				break;
			}
			return completed ? output : null;
		}
	}

	void set_method (Method _method) {
		method = _method;
		if (method == Method.GAMMA) {
			var operation = operations[method] as Operation;
			SignalHandler.block (amount_adjustment, amount_changed_id);
			amount_adjustment.configure (operation.amount, MIN_GAMMA, MAX_GAMMA, 0.1, 0.1, 0);
			SignalHandler.unblock (amount_adjustment, amount_changed_id);
		}
		else if (method == Method.LINEAR) {
			var operation = operations[method] as Operation;
			SignalHandler.block (amount_adjustment, amount_changed_id);
			amount_adjustment.configure (operation.amount, MIN_LINEAR, MAX_LINEAR, 0.1, 0.1, 0);
			SignalHandler.unblock (amount_adjustment, amount_changed_id);
		}
		else if (method == Method.CURVE) {
			var operation = operations[method] as BrightnessCurve;
			SignalHandler.block (amount_adjustment, amount_changed_id);
			amount_adjustment.configure (operation.amount, MIN_CURVE, MAX_CURVE, 1, 5, 0);
			SignalHandler.unblock (amount_adjustment, amount_changed_id);
		}
		queue_update_preview ();
	}

	construct {
		title = _("Brightness");
		icon_name = "gth-adjust-brightness-symbolic";

		action_group = new SimpleActionGroup ();
		var action = new SimpleAction.stateful ("method", VariantType.STRING, new Variant.string ("gamma"));
		action.activate.connect ((_action, param) => {
			var method = param.get_string ();
			action.set_state (method);
			set_method ((method == "gamma") ? Method.GAMMA : (method == "curve") ? Method.CURVE : Method.LINEAR);
		});
		action_group.add_action (action);
	}

	Gtk.Builder builder;
	ImageOperation[] operations;
	Method method;
	SimpleActionGroup action_group;
	unowned Gtk.Adjustment amount_adjustment;
	unowned Gtk.Label amount_label;
	ulong amount_changed_id = 0;

	const double MIN_GAMMA = 0;
	const double MAX_GAMMA = 2;
	const double MIN_LINEAR = -1.0;
	const double MAX_LINEAR = 1.0;
	const double MIN_CURVE = -100;
	const double MAX_CURVE = 100;
}
