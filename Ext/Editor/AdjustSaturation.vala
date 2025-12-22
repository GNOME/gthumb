public class Gth.AdjustSaturation : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/adjust-saturation.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);
		window.editor.sidebar.insert_action_group ("saturation", action_group);

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.resized.connect (() => update_preview_on_resize ());
		add_default_controllers (image_view);

		window.editor.set_content (image_view);

		amount_adjustment = builder.get_object ("amount_adjustment") as Gtk.Adjustment;
		amount_changed_id = amount_adjustment.value_changed.connect (() => {
			if (operation != null) {
				amount = adj_to_amount (amount_adjustment.value);
				queue_update_preview ();
			}
		});

		amount_label = builder.get_object ("amount_label") as Gtk.Label;
		amount_adjustment.value_changed.connect ((local_adj) => {
			var int_value = (int) Math.round (local_adj.value);
			amount_label.label = (int_value == 0) ? "0" : "%+d".printf (int_value);
		});

		var reset_button = builder.get_object ("reset_button") as Gtk.Button;
		reset_button.clicked.connect (() => {
			if (operation != null) {
				amount = 0.0;
				queue_update_preview ();
			}
		});

		var preview_switch = builder.get_object ("preview") as Adw.SwitchRow;
		preview_switch.notify["active"].connect ((obj, param) => {
			var local_switch = obj as Adw.SwitchRow;
			show_preview (local_switch.active);
		});

		operation = new Operation (Method.BRIGHTNESS);
		amount = DEFAULT_VALUE;
	}

	public override void before_deactivate () {
		window.editor.sidebar.insert_action_group ("saturation", null);
		builder = null;
	}

	public override ImageOperation? get_operation () {
		if (operation != null) {
			operation.amount = amount;
		}
		return operation;
	}

	public override void update_options_from_operation (ImageOperation image_operation) {
		var operation = image_operation as Operation;
		SignalHandler.block (amount_adjustment, amount_changed_id);
		amount_adjustment.set_value (amount_to_adj (operation.amount));
		SignalHandler.unblock (amount_adjustment, amount_changed_id);
	}

	double adj_to_amount (double adj) {
		return - (adj / 100.0);
	}

	double amount_to_adj (double amount) {
		return Math.round ((- amount) * 100.0);
	}

	enum Method {
		NONE = 0,
		BRIGHTNESS,
		SATURATION,
		AVERAGE;
	}

	class Operation : ImageOperation {
		public Method method;
		public double amount;
		public double default_amount;

		public Operation (Method _method, double _amount = 1.0) {
			method = _method;
			default_amount = _amount;
			amount = _amount;
		}

		public override Gth.Image? execute (Image input, Cancellable cancellable) {
			if (input == null) {
				return null;
			}
			var output = input.dup ();
			switch (method) {
			case Method.BRIGHTNESS:
				output.grayscale (0.2125, 0.7154, 0.072, amount);
				break;
			case Method.AVERAGE:
				output.grayscale (0.3333, 0.3333, 0.3333, amount);
				break;
			case Method.SATURATION:
				output.grayscale_saturation (amount);
				break;
			default:
				break;
			}
			return output;
		}
	}

	construct {
		title = _("Saturation");
		icon_name = "gth-adjust-brightness-symbolic";

		action_group = new SimpleActionGroup ();
		var action = new SimpleAction.stateful ("method", VariantType.STRING, new Variant.string ("brightness"));
		action.activate.connect ((_action, param) => {
			var method = param.get_string ();
			action.set_state (method);
			if (method == "brightness") {
				operation.method = Method.BRIGHTNESS;
			}
			else if (method == "saturation") {
				operation.method = Method.SATURATION;
			}
			else if (method == "average") {
				operation.method = Method.AVERAGE;
			}
			queue_update_preview ();
		});
		action_group.add_action (action);
	}

	Gtk.Builder builder;
	Operation operation;
	SimpleActionGroup action_group;
	unowned Gtk.Adjustment amount_adjustment;
	unowned Gtk.Label amount_label;
	ulong amount_changed_id = 0;
	double amount;

	const double DEFAULT_VALUE = 0.0;
}
