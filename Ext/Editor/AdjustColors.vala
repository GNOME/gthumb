public class Gth.AdjustColors : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/adjust-colors.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.resized.connect (() => update_preview_on_resize ());
		add_default_controllers (image_view);

		window.editor.set_content (image_view);

		red_adjustment = builder.get_object ("red_adjustment") as Gtk.Adjustment;
		red_changed_id = red_adjustment.value_changed.connect ((local_adj) => {
			operation.red_amount = adj_to_amount (local_adj.value);
			operation.update_points ();
			queue_update_preview ();
		});

		green_adjustment = builder.get_object ("green_adjustment") as Gtk.Adjustment;
		green_changed_id = green_adjustment.value_changed.connect ((local_adj) => {
			operation.green_amount = adj_to_amount (local_adj.value);
			operation.update_points ();
			queue_update_preview ();
		});

		blue_adjustment = builder.get_object ("blue_adjustment") as Gtk.Adjustment;
		blue_changed_id = blue_adjustment.value_changed.connect ((local_adj) => {
			operation.blue_amount = adj_to_amount (local_adj.value);
			operation.update_points ();
			queue_update_preview ();
		});

		value_adjustment = builder.get_object ("value_adjustment") as Gtk.Adjustment;
		value_changed_id = value_adjustment.value_changed.connect ((local_adj) => {
			operation.value_amount = adj_to_amount (local_adj.value);
			operation.update_points ();
			queue_update_preview ();
		});

		unowned var reset_button = builder.get_object ("reset_button") as Gtk.Button;
		reset_button.clicked.connect (() => reset_amount ());

		var preview_switch = builder.get_object ("preview") as Adw.SwitchRow;
		preview_switch.notify["active"].connect ((obj, param) => {
			var local_switch = obj as Adw.SwitchRow;
			show_preview (local_switch.active);
		});

		operation = new Operation ();
	}

	public override void before_deactivate () {
		builder = null;
	}

	public override ImageOperation? get_operation () {
		return operation;
	}

	public override void update_options_from_operation (ImageOperation image_operation) {
		var operation = image_operation as Operation;
		SignalHandler.block (red_adjustment, red_changed_id);
		red_adjustment.set_value (amount_to_adj (operation.red_amount));
		SignalHandler.unblock (red_adjustment, red_changed_id);

		SignalHandler.block (green_adjustment, green_changed_id);
		green_adjustment.set_value (amount_to_adj (operation.green_amount));
		SignalHandler.unblock (green_adjustment, green_changed_id);

		SignalHandler.block (blue_adjustment, blue_changed_id);
		blue_adjustment.set_value (amount_to_adj (operation.blue_amount));
		SignalHandler.unblock (blue_adjustment, blue_changed_id);

		SignalHandler.block (value_adjustment, value_changed_id);
		value_adjustment.set_value (amount_to_adj (operation.value_amount));
		SignalHandler.unblock (value_adjustment, value_changed_id);
	}

	void reset_amount () {
		operation.red_amount = 0;
		operation.green_amount = 0;
		operation.blue_amount = 0;
		operation.value_amount = 0;
		update_preview ();
	}

	class Operation : CurveOperation {
		public double red_amount;
		public double green_amount;
		public double blue_amount;
		public double value_amount;

		public Operation () {
			red_amount = 0.0;
			green_amount = 0.0;
			blue_amount = 0.0;
			value_amount = 0.0;
			update_points ();
		}

		public void update_points () {
			var value_point = Point.interpolate (Point (127, 127), Point (77, 169), value_amount);
			var red_point = Point.interpolate (Point (127, 127), Point (77, 169), red_amount);
			var green_point = Point.interpolate (Point (127, 127), Point (77, 169), green_amount);
			var blue_point = Point.interpolate (Point (127, 127), Point (77, 169), blue_amount);
			points = Points () {
				value = { Point (0, 0), value_point, Point (255, 255) },
				red   = { Point (0, 0), red_point, Point (255, 255) },
				green = { Point (0, 0), green_point, Point (255, 255) },
				blue  = { Point (0, 0), blue_point, Point (255, 255) },
			};
		}
	}

	double adj_to_amount (double value) {
		return value / 100.0;
	}

	double amount_to_adj (double value) {
		return value * 100.0;
	}

	construct {
		title = _("Colors");
		icon_name = "gth-adjust-colors-symbolic";
	}

	Gtk.Builder builder;
	Operation operation;
	unowned Gtk.Adjustment red_adjustment;
	unowned Gtk.Adjustment green_adjustment;
	unowned Gtk.Adjustment blue_adjustment;
	unowned Gtk.Adjustment value_adjustment;
	ulong red_changed_id = 0;
	ulong green_changed_id = 0;
	ulong blue_changed_id = 0;
	ulong value_changed_id = 0;
}
