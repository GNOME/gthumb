public class Gth.SpecialEffects : ImageTool {
	public override void activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/special-effects.ui");
		window.editor.sidebar.child = builder.get_object ("options") as Gtk.Widget;

		filter_grid = builder.get_object ("filter_grid") as Gth.FilterGrid;
		/* Translators: this is the name of a filter that produces warmer colors. */
		filter_grid.add (Method.WARMER, new Operation (Method.WARMER), _("Warmer"));
		/* Translators: this is the name of a filter that produces cooler colors. */
		filter_grid.add (Method.COOLER, new Operation (Method.COOLER), _("Cooler"));
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
		filter_grid.activate (Method.WARMER);
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
		WARMER,
		COOLER,
	}

	struct Points {
		Point[] value;
		Point[] red;
		Point[] green;
		Point[] blue;

		public long[,] to_value_map () {
			var value_map = new long[4, 256];
			for (var channel = Channel.VALUE; channel <= Channel.BLUE; channel += 1) {
				var curve = new Bezier (get_points (channel));
				for (var v = 0; v <= 255; v++) {
					var u = curve.eval (v);
					if (channel != Channel.VALUE) {
						u = value_map[Channel.VALUE, (int) u];
					}
					value_map[channel, v] = (long) u;
				}
			}
			return value_map;
		}

		unowned Point[] get_points (Channel channel) {
			if (channel == Channel.VALUE) {
				return value;
			}
			else if (channel == Channel.RED) {
				return red;
			}
			else if (channel == Channel.GREEN) {
				return green;
			}
			else if (channel == Channel.BLUE) {
				return blue;
			}
			return null;
		}
	}

	class Operation : ImageOperation {
		public Operation (Method _method) {
			set_method (_method);
		}

		public void set_method (Method _method) {
			method = _method;
			switch (method) {
			case Method.WARMER:
				points = Points () {
					value = null,
					red   = { { 0, 0 }, { 117, 136 }, { 255, 255 } },
					green = null,
					blue  = { { 0, 0 }, { 136, 119 }, { 255, 255 } },
				};
				break;
			case Method.COOLER:
				points = Points () {
					value = null,
					red   = { { 0, 0 }, { 136, 119 }, { 255, 255 } },
					green = null,
					blue  = { { 0, 0 }, { 117, 136 }, { 255, 255 } },
				};
				break;
			default:
				points = Points ();
				break;
			}
			value_map = points.to_value_map ();
		}

		public override Gth.Image? execute (Image input, Cancellable cancellable) {
			if (input == null) {
				return null;
			}
			var output = input.dup ();
			output.apply_value_map (value_map);
			return output;
		}

		Method method;
		Points points;
		long [,] value_map;
	}

	construct {
		title = _("Special Effects");
		icon_name = "gth-special-effects-symbolic";
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
