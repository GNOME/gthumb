public class Gth.AdjustContrast : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/adjust-contrast.ui");
		window.editor.sidebar.child = builder.get_object ("options") as Gtk.Widget;

		filter_grid = builder.get_object ("filter_grid") as Gth.FilterGrid;
		// Translators: filter that adjust the contrast balancing the white component.
		filter_grid.add (Method.STRETCH, new Operation (Method.STRETCH, 0.005), _("Balanced"));
		// Translators: filter that makes a linear tranformation of the colors.
		filter_grid.add (Method.LINEAR, new Operation (Method.LINEAR), _("Linear"));
		// Translators: filter that equalizes the histogram.
		filter_grid.add (Method.EQUALIZE, new Operation (Method.EQUALIZE), _("Equalize"));
		filter_grid.activated.connect ((id) => {
			var operation = filter_grid.get_operation (id) as Operation;
			if (operation.method == Method.STRETCH) {
				window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
				var scale = builder.get_object ("amount_scale") as Gtk.Scale;
				scale.digits = 0;
				scale.clear_marks ();
				scale.add_mark (crop_size_to_adj (0.005), Gtk.PositionType.BOTTOM, null);
				scale.add_mark (crop_size_to_adj (0.015), Gtk.PositionType.BOTTOM, null);
				scale.add_mark (crop_size_to_adj (0.025), Gtk.PositionType.BOTTOM, null);
				SignalHandler.block (amount_adjustment, amount_changed_id);
				amount_adjustment.configure (crop_size_to_adj (operation.amount),
					crop_size_to_adj (MIN_STRETCH),
					crop_size_to_adj (MAX_STRETCH), 1, 1, 0);
				SignalHandler.unblock (amount_adjustment, amount_changed_id);
			}
			else if (operation.method == Method.LINEAR) {
				window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
				var scale = builder.get_object ("amount_scale") as Gtk.Scale;
				scale.digits = 2;
				scale.clear_marks ();
				SignalHandler.block (amount_adjustment, amount_changed_id);
				amount_adjustment.configure (operation.amount, MIN_LINEAR, MAX_LINEAR, 0.1, 0.1, 0);
				SignalHandler.unblock (amount_adjustment, amount_changed_id);
			}
			else if (operation.method == Method.EQUALIZE) {
				window.editor.set_action_bar (builder.get_object ("equalize_action_bar") as Gtk.Widget);
			}
			queue_update_preview ();
		});

		window.editor.content.child = builder.get_object ("image_view") as Gtk.Widget;
		window.editor.content.add_css_class ("image-view");

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.resized.connect (() => update_preview_on_resize ());
		add_default_controllers (image_view);

		amount_adjustment = builder.get_object ("amount_adjustment") as Gtk.Adjustment;
		amount_changed_id = amount_adjustment.value_changed.connect (() => {
			var id = filter_grid.get_activated ();
			var operation = filter_grid.get_operation (id) as Operation;
			if (operation != null) {
				if (operation.method == Method.STRETCH) {
					operation.amount = adj_to_crop_size (amount_adjustment.value);
				}
				else if (operation.method == Method.LINEAR) {
					operation.amount = -amount_adjustment.value;
				}
				queue_update_preview ();
			}
		});

		var linear_switch = builder.get_object ("equalize_linear") as Adw.SwitchRow;
		linear_switch.notify["active"].connect ((obj, param) => {
			var local_switch = obj as Adw.SwitchRow;
			var id = filter_grid.get_activated ();
			var operation = filter_grid.get_operation (id) as Operation;
			if ((operation != null) && (operation.method == Method.EQUALIZE)) {
				operation.equalize_linear = local_switch.active;
				queue_update_preview ();
			}
		});

		unowned var reset_button = builder.get_object ("reset_button") as Gtk.Button;
		reset_button.clicked.connect (() => reset_amount ());

		update_thumbnails ();
		filter_grid.activate (Method.STRETCH);
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
		var operation = image_operation as Operation;
		var method = (Method) filter_grid.get_activated ();
		if (method.has_adjustment ()) {
			SignalHandler.block (amount_adjustment, amount_changed_id);
			if (method == Method.STRETCH) {
				amount_adjustment.set_value (crop_size_to_adj (operation.amount));
			}
			else if (method == Method.LINEAR) {
				amount_adjustment.set_value (-operation.amount);
			}
			SignalHandler.unblock (amount_adjustment, amount_changed_id);
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

	void reset_amount () {
		var id = filter_grid.get_activated ();
		var operation = filter_grid.get_operation (id) as Operation;
		if (operation != null) {
			operation.amount = operation.default_amount;
			update_preview ();
		}
	}

	public static double adj_to_crop_size (double x) {
		return x / 1000;
	}

	public static double crop_size_to_adj (double x) {
		return 1000.0 * x;
	}

	enum Method {
		NONE = 0,
		STRETCH,
		EQUALIZE,
		LINEAR;

		public bool has_adjustment () {
			return (this == STRETCH) || (this == LINEAR);
		}
	}

	class Operation : ImageOperation {
		public Method method;
		public double amount;
		public double default_amount;
		public bool equalize_linear;

		public Operation (Method _method, double _amount = 0.0) {
			method = _method;
			amount = _amount;
			default_amount = _amount;
			equalize_linear = false;
		}

		public override Gth.Image? execute (Image input, Cancellable cancellable) {
			if (input == null) {
				return null;
			}
			var output = input.dup ();
			switch (method) {
			case Method.STRETCH:
				output.stretch_histogram (amount);
				break;
			case Method.EQUALIZE:
				output.equalize_histogram (equalize_linear);
				break;
			case Method.LINEAR:
				output.adjust_contrast (amount);
				break;
			default:
				break;
			}
			return output;
		}
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
	const double MIN_STRETCH = 0;
	const double MAX_STRETCH = 0.030;
	const double MIN_LINEAR = -1.0;
	const double MAX_LINEAR = 1.0;
}
