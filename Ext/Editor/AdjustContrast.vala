public class Gth.AdjustContrast : ImageTool {
	public override void activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/adjust-contrast.ui");
		window.editor.sidebar.child = builder.get_object ("options") as Gtk.Widget;

		filter_grid = builder.get_object ("filter_grid") as Gth.FilterGrid;
		filter_grid.add (Method.STRETCH, new Operation (Method.STRETCH, 0.005), _("Balanced"));
		filter_grid.add (Method.EQUALIZE_SQUARE_ROOT, new Operation (Method.EQUALIZE_SQUARE_ROOT), _("Equalize"));
		filter_grid.add (Method.EQUALIZE_LINEAR, new Operation (Method.EQUALIZE_LINEAR), _("Linear"));
		filter_grid.activated.connect ((id) => {
			var method = (Method) id;
			if (method == Method.STRETCH) {
				window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
				var scale = builder.get_object ("amount_scale") as Gtk.Scale;
				scale.clear_marks ();
				scale.add_mark (crop_size_to_adj (0.005), Gtk.PositionType.BOTTOM, null);
				scale.add_mark (crop_size_to_adj (0.015), Gtk.PositionType.BOTTOM, null);
				scale.add_mark (crop_size_to_adj (0.025), Gtk.PositionType.BOTTOM, null);
				SignalHandler.block (amount_adjustment, amount_changed_id);
				amount_adjustment.configure (0, crop_size_to_adj (MIN_STRETCH),
					crop_size_to_adj (MAX_STRETCH), 1, 1, 0);
				SignalHandler.unblock (amount_adjustment, amount_changed_id);
			}
			else {
				window.editor.set_action_bar (null);
			}
			update_preview (method);
		});

		window.editor.content.child = builder.get_object ("image_view") as Gtk.Widget;
		window.editor.content.add_css_class ("image-view");

		original = viewer.image_view.image;
		resized = null;

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.default_zoom_type = ZoomType.MAXIMIZE_IF_LARGER;
		image_view.image = original;
		image_view.resized.connect (() => {
			if (resized == null) {
				update_preview ((Method) filter_grid.get_activated ());
			}
		});

		amount_adjustment = builder.get_object ("amount_adjustment") as Gtk.Adjustment;
		amount_changed_id = amount_adjustment.value_changed.connect (() => {
			var id = filter_grid.get_activated ();
			var operation = filter_grid.get_operation (id) as Operation;
			if (operation != null) {
				operation.amount = adj_to_crop_size (amount_adjustment.value);
				update_preview ((Method) id);
			}
		});

		unowned var reset_button = builder.get_object ("reset_button") as Gtk.Button;
		reset_button.clicked.connect (() => reset_amount ());

		update_thumbnails ();
		filter_grid.activate (Method.STRETCH);
	}

	public override void deactivate () {
		if (thumbnails_job != null) {
			thumbnails_job.cancel ();
		}
		if (preview_job != null) {
			preview_job.cancel ();
		}
		builder = null;
	}

	public override ImageOperation? get_operation () {
		return filter_grid.get_active_operation ();
	}

	void update_preview (Method method) {
		if (preview_job != null) {
			preview_job.cancel ();
		}
		var job = window.new_job ("Update Preview");
		preview_job = job;
		preview_filter_async.begin (method, job.cancellable, (_obj, res) => {
			try {
				image_view.image = preview_filter_async.end (res);
			}
			catch (Error error) {
				window.show_error (error);
			}
			finally {
				job.done ();
				if (job == preview_job) {
					preview_job = null;
				}
			}
		});
	}

	uint get_preview_size () {
		int width = image_view.get_width ();
		int height = image_view.get_height ();
		return (uint) ((original.width > original.height) ? width : height);
	}

	async Image? preview_filter_async (Method method, Cancellable cancellable) throws Error {
		if (resized == null) {
			resized = original; //yield original.resize_async (get_preview_size (), ResizeFlags.DEFAULT, ScaleFilter.BOX, cancellable);
		}
		var operation = filter_grid.get_operation ((int) method) as Operation;
		if (operation == null) {
			return resized;
		}
		if (method == Method.STRETCH) {
			SignalHandler.block (amount_adjustment, amount_changed_id);
			amount_adjustment.set_value (crop_size_to_adj (operation.amount));
			SignalHandler.unblock (amount_adjustment, amount_changed_id);
		}
		return yield app.image_editor.exec_operation (resized, operation, cancellable);
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
			update_preview ((Method) id);
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
		EQUALIZE_LINEAR,
		EQUALIZE_SQUARE_ROOT;
	}

	class Operation : ImageOperation {
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
			switch (method) {
			case Method.STRETCH:
				output.stretch_histogram (amount);
				break;
			case Method.EQUALIZE_LINEAR:
				output.equalize_histogram (true);
				break;
			case Method.EQUALIZE_SQUARE_ROOT:
				output.equalize_histogram (false);
				break;
			default:
				break;
			}
			return output;
		}

		Method method;
	}

	construct {
		title = _("Adjust Contrast");
		icon_name = "gth-adjust-contrast-symbolic";
	}

	Gtk.Builder builder;
	Gth.FilterGrid filter_grid;
	Job thumbnails_job = null;
	Job preview_job = null;
	unowned ImageView image_view;
	Image original;
	Image resized;
	unowned Gtk.Adjustment amount_adjustment;
	ulong amount_changed_id = 0;

	const uint THUMBNAIL_SIZE = 140;
	const double MIN_STRETCH = 0;
	const double MAX_STRETCH = 0.030;
}
