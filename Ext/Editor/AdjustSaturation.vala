public class Gth.AdjustSaturation : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/adjust-saturation.ui");
		window.editor.sidebar.child = builder.get_object ("options") as Gtk.Widget;
		window.editor.content.child = builder.get_object ("image_view") as Gtk.Widget;
		window.editor.content.add_css_class ("image-view");

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.resized.connect (() => update_preview_on_resize ());
		add_default_controllers (image_view);

		amount_adjustment = builder.get_object ("amount_adjustment") as Gtk.Adjustment;
		amount_changed_id = amount_adjustment.value_changed.connect (() => {
			if (operation != null) {
				amount = adj_to_amount (amount_adjustment.value);
				queue_update_preview ();
			}
		});

		var reset_button = builder.get_object ("reset_button") as Gtk.Button;
		reset_button.clicked.connect (() => {
			if (operation != null) {
				amount = 0.0;
				var local_combo = builder.get_object ("method") as Adw.ComboRow;
				local_combo.selected = 0;
				queue_update_preview ();
			}
		});

		var preview_switch = builder.get_object ("preview") as Adw.SwitchRow;
		preview_switch.notify["active"].connect ((obj, param) => {
			var local_switch = obj as Adw.SwitchRow;
			show_preview (local_switch.active);
		});

		operation = new Operation (Method.BRIGHTNESS);

		var method_combo = builder.get_object ("method") as Adw.ComboRow;
		method_combo.notify["selected"].connect ((obj, param) => {
			var local_combo = obj as Adw.ComboRow;
			operation.method = (Method) (local_combo.selected + 1);
			queue_update_preview ();
		});

		amount = DEFAULT_VALUE;
	}

	public override void before_deactivate () {
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
		AVARAGE;
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
			case Method.AVARAGE:
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
	}

	Gtk.Builder builder;
	Operation operation;
	unowned Gtk.Adjustment amount_adjustment;
	ulong amount_changed_id = 0;
	double amount;

	const double DEFAULT_VALUE = 0.0;
}
