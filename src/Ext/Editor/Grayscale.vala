public class Gth.Grayscale : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/org/gnome/gthumb/ui/grayscale.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);

		filter_grid = builder.get_object ("filter_grid") as Gth.FilterGrid;
		filter_grid.add (Method.BRIGHTNESS, new Operation (Method.BRIGHTNESS), _("Brightness"));
		filter_grid.add (Method.SATURATION, new Operation (Method.SATURATION), _("Lightness"));
		filter_grid.add (Method.AVERAGE, new Operation (Method.AVERAGE), _("Average"));
		filter_grid.activated.connect ((id) => queue_update_preview ());

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		add_default_controllers (image_view);
		image_view.image = viewer.image_view.image;
		image_view.set_first_state_from_view (viewer.image_view);

		window.editor.set_content (image_view);

		update_thumbnails ();
		filter_grid.activate_filter (Method.BRIGHTNESS);
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
		BRIGHTNESS,
		SATURATION,
		AVERAGE;
	}

	class Operation : ImageOperation {
		public Operation (Method _method) {
			method = _method;
		}

		public override Gth.Image? execute (Image input, Cancellable cancellable, bool for_preview = false) {
			if (input == null) {
				return null;
			}
			var output = input.dup ();
			var completed = false;
			switch (method) {
			case Method.BRIGHTNESS:
				completed = output.grayscale (0.2125, 0.7154, 0.072, 1.0, cancellable);
				break;
			case Method.AVERAGE:
				completed = output.grayscale (0.3333, 0.3333, 0.3333, 1.0, cancellable);
				break;
			case Method.SATURATION:
				completed = output.grayscale_saturation (1.0, cancellable);
				break;
			default:
				break;
			}
			return completed ? output : null;
		}

		Method method;
	}

	construct {
		title = _("Grayscale");
		icon_name = "gth-grayscale-symbolic";
	}

	Gtk.Builder builder;
	Gth.FilterGrid filter_grid;
	Job thumbnails_job = null;

	const uint THUMBNAIL_SIZE = 140;
}
