public class Gth.SharpenImage : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/sharpen-image.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);
		window.editor.set_content (builder.get_object ("content") as Gtk.Widget);

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		add_default_controllers (image_view);

		operation = new Operation ();

		amount_adjustment = builder.get_object ("amount_adjustment") as Gtk.Adjustment;
		amount_changed_id = amount_adjustment.value_changed.connect ((local_adj) => {
			operation.amount = local_adj.value;
			queue_update_preview ();
		});
		operation.amount = amount_adjustment.value;

		radius_adjustment = builder.get_object ("radius_adjustment") as Gtk.Adjustment;
		radius_changed_id = radius_adjustment.value_changed.connect ((local_adj) => {
			operation.radius = local_adj.value;
			queue_update_preview ();
		});
		operation.radius = radius_adjustment.value;

		threshold_adjustment = builder.get_object ("threshold_adjustment") as Gtk.Adjustment;
		threshold_changed_id = threshold_adjustment.value_changed.connect ((local_adj) => {
			operation.threshold = local_adj.value;
			queue_update_preview ();
		});
		operation.threshold = threshold_adjustment.value;

		var reset_button = builder.get_object ("reset_button") as Gtk.Button;
		reset_button.clicked.connect (() => reset_options ());

		var preview_switch = builder.get_object ("preview") as Adw.SwitchRow;
		preview_switch.notify["active"].connect ((obj, param) => {
			var local_switch = obj as Adw.SwitchRow;
			show_preview (local_switch.active);
		});

		reset_options ();
	}

	public override void before_deactivate () {
		builder = null;
	}

	public override ImageOperation? get_operation () {
		return operation;
	}

	void reset_options () {
		SignalHandler.block (amount_adjustment, amount_changed_id);
		SignalHandler.block (radius_adjustment, radius_changed_id);
		SignalHandler.block (threshold_adjustment, threshold_changed_id);
		operation.amount = amount_adjustment.value = DEFAULT_AMOUNT;
		operation.radius = radius_adjustment.value = DEFAULT_RADIUS;
		operation.threshold = threshold_adjustment.value = DEFAULT_THRESHOLD;
		SignalHandler.unblock (amount_adjustment, amount_changed_id);
		SignalHandler.unblock (radius_adjustment, radius_changed_id);
		SignalHandler.unblock (threshold_adjustment, threshold_changed_id);
		queue_update_preview ();
	}

	class Operation : ImageOperation {
		public double amount;
		public double radius;
		public double threshold;

		public override Gth.Image? execute (Image input, Cancellable cancellable) {
			if (input == null) {
				return null;
			}
			var output = input.dup ();
			output.sharpen (amount, (int) radius, threshold, cancellable);
			return output;
		}
	}

	construct {
		title = _("Focus");
		icon_name = "gth-sharpen-symbolic";
	}

	Gtk.Builder builder;
	Operation operation;
	unowned Gtk.Adjustment amount_adjustment;
	unowned Gtk.Adjustment radius_adjustment;
	unowned Gtk.Adjustment threshold_adjustment;
	ulong amount_changed_id = 0;
	ulong radius_changed_id = 0;
	ulong threshold_changed_id = 0;

	const double DEFAULT_AMOUNT = 0.5;
	const double DEFAULT_RADIUS = 2;
	const double DEFAULT_THRESHOLD = 0;
}
