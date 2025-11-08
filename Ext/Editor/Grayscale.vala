public class Gth.Grayscale : ImageTool {
	public override void activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/grayscale.ui");
		window.editor.sidebar.child = builder.get_object ("options") as Gtk.Widget;

		filter_grid = builder.get_object ("filter_grid") as Gth.FilterGrid;
		filter_grid.add (Method.BRIGHTNESS, new Operation (Method.BRIGHTNESS), _("Brightness"));
		filter_grid.add (Method.SATURATION, new Operation (Method.SATURATION), _("Saturation"));
		filter_grid.add (Method.AVARAGE, new Operation (Method.AVARAGE), _("Average"));
		filter_grid.activated.connect ((id) => update_preview ((Method) id));

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

		update_thumbnails ();
		filter_grid.activate (Method.BRIGHTNESS);
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
		BRIGHTNESS,
		SATURATION,
		AVARAGE;
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
			case Method.BRIGHTNESS:
				output.grayscale (0.2125, 0.7154, 0.072);
				break;
			case Method.AVARAGE:
				output.grayscale (0.3333, 0.3333, 0.3333);
				break;
			case Method.SATURATION:
				output.grayscale_saturation ();
				break;
			default:
				break;
			}
			return output;
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
	Job preview_job = null;
	unowned ImageView image_view;
	Image original;
	Image resized;

	const uint THUMBNAIL_SIZE = 140;
}
