public class Gth.AdjustContrast : ImageTool {
	public override void activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/adjust-contrast.ui");
		window.editor.sidebar.child = builder.get_object ("options") as Gtk.Widget;

		filter_grid = builder.get_object ("filter_grid") as Gth.FilterGrid;
		filter_grid.add (Method.STRETCH_0_5, new Operation (Method.STRETCH_0_5), _("Balanced"));
		filter_grid.add (Method.STRETCH_1_5, new Operation (Method.STRETCH_1_5), _("Extended"));
		filter_grid.add (Method.EQUALIZE_SQUARE_ROOT, new Operation (Method.EQUALIZE_SQUARE_ROOT), _("Equalize"));
		filter_grid.add (Method.EQUALIZE_LINEAR, new Operation (Method.EQUALIZE_LINEAR), _("Linear"));
		filter_grid.activated.connect ((id) => update_preview ((Method) id));

		window.editor.content.child = builder.get_object ("image_view") as Gtk.Widget;
		window.editor.content.add_css_class ("image-view");

		original = viewer.image_view.image;
		resized = null;

		histogram_view = builder.get_object ("histogram_view") as Gth.HistogramView;
		histogram_view.histogram = histogram;

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.default_zoom_type = ZoomType.MAXIMIZE_IF_LARGER;
		image_view.image = original;
		image_view.resized.connect (() => {
			if (resized == null) {
				update_preview ((Method) filter_grid.get_activated ());
			}
		});

		update_thumbnails ();
		filter_grid.activate (Method.STRETCH_0_5);
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
		var method = (Method) filter_grid.get_activated ();
		if (method == Method.NONE) {
			return null;
		}
		return new Operation (method);
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
				histogram.update (image_view.image);
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
		var operation = new Operation (method);
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

	enum Method {
		NONE = 0,
		STRETCH_0_5,
		STRETCH_1_5,
		EQUALIZE_LINEAR,
		EQUALIZE_SQUARE_ROOT;

		public double get_crop_size () {
			return CROP_SIZE[this];
		}

		const double[] CROP_SIZE = { 0.0, 0.005, 0.015 };
	}

	class Operation : ImageOperation {
		public Operation (Method _method) {
			method = _method;
		}

		public override Gth.Image? execute (Image input, Cancellable cancellable) {
			if (input == null) {
				return null;
			}
			var output = input.dup ();
			switch (method) {
			case Method.STRETCH_0_5, Method.STRETCH_1_5:
				output.stretch_histogram (method.get_crop_size ());
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
		histogram = new Histogram ();
	}

	Gtk.Builder builder;
	Gth.FilterGrid filter_grid;
	Job thumbnails_job = null;
	Job preview_job = null;
	unowned ImageView image_view;
	unowned HistogramView histogram_view;
	Image original;
	Image resized;
	Histogram histogram;

	const uint THUMBNAIL_SIZE = 140;
}
