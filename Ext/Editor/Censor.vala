public class Gth.Censor : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/censor-image.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);
		// window.editor.sidebar.insert_action_group ("censor", action_group);

		filter_grid = builder.get_object ("filter_grid") as Gth.FilterGrid;

		var image_size = uint.max (viewer.image_view.image.width, viewer.image_view.image.height);
		var default_pixels = (int) ((double) image_size * 0.75 / 100);

		// Translators: this is the name of an image filter.
		filter_grid.add (Effect.PIXELIZE, new Pixelize (default_pixels), _("Mosaic"));

		// Translators: this is the name of an image filter.
		filter_grid.add (Effect.BLUR, new Blur (), _("Blur"));

		filter_grid.add (Effect.COLOR, new SolidColor (), _("Color"));

		filter_grid.activated.connect ((id) => {
			var method = (Effect) id;
			var operation = filter_grid.get_operation (method) as ParametricOperation;
			if ((operation != null) && operation.has_parameter ()) {
				window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
			}
			else if (method == Effect.COLOR) {
				window.editor.set_action_bar (builder.get_object ("color_action_bar") as Gtk.Widget);
			}
			else {
				window.editor.set_action_bar (null);
			}
			queue_update_preview ();
		});

		var color_row = builder.get_object ("color_row") as Adw.ActionRow;
		color_row.activated.connect (() => {
			var local_job = window.new_job (_("Color"));
			var selector = new Gth.ColorSelector ();
			var operation = filter_grid.get_operation (Effect.COLOR) as SolidColor;
			color_job = local_job;
			selector.select_color.begin (window, operation.color, local_job.cancellable, (_obj, res) => {
				try {
					var color = selector.select_color.end (res);
					if (color != null) {
						var color_preview = builder.get_object ("color_preview") as Gth.ColorPreview;
						color_preview.color = color;
						operation.color = color;
						queue_update_preview ();
					}
				}
				catch (Error e) {
					// Ignore.
				}
				local_job.done ();
				if (local_job == color_job) {
					color_job = null;
				}
			});
		});

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.default_zoom_type = ZoomType.MAXIMIZE_IF_LARGER;
		image_view.image = viewer.image_view.image;

		window.editor.set_content (image_view);

		selector = new MaskSelector ();
		selector.grid_type = GridType.NONE;
		image_view.controller = selector;

		amount_adjustment = builder.get_object ("amount_adjustment") as Gtk.Adjustment;
		amount_changed_id = amount_adjustment.value_changed.connect (() => {
			var id = filter_grid.get_activated ();
			var operation = filter_grid.get_operation (id) as ParametricOperation;
			if (operation != null) {
				operation.set_amount (amount_adjustment.value / 100.0);
				queue_update_preview ();
			}
		});

		unowned var reset_button = builder.get_object ("reset_button") as Gtk.Button;
		reset_button.clicked.connect (() => reset_amount ());

		update_thumbnails ();
		filter_grid.activate_filter (Effect.PIXELIZE);
	}

	public override void before_deactivate () {
		// window.editor.sidebar.insert_action_group ("censor", null);
		if (color_job != null) {
			color_job.cancel ();
		}
		if (thumbnails_job != null) {
			thumbnails_job.cancel ();
		}
		image_view.controller = null;
		builder = null;
	}

	public override ImageOperation? get_operation () {
		var filter = filter_grid.get_active_operation ();
		if (filter == null) {
			return null;
		}
		var operation = new MaskedFilter (filter);
		operation.mask = selector.selection;
		operation.filtered = selector.filtered;
		return operation;
	}

	public override void update_options_from_operation (ImageOperation image_operation) {
		var operation = image_operation as ParametricOperation;
		SignalHandler.block (amount_adjustment, amount_changed_id);
		amount_adjustment.set_value (Math.round (operation.get_amount () * 100.0));
		SignalHandler.unblock (amount_adjustment, amount_changed_id);
	}

	public override void update_preview () {
		if (image_view == null) {
			return;
		}
		var operation = filter_grid.get_active_operation ();
		if (operation != null) {
			update_options_from_operation (operation);
		}
		selector.filter_operation = with_preview ? operation : null;
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
		var operation = filter_grid.get_operation (id) as ParametricOperation;
		if (operation != null) {
			operation.reset_amount ();
			update_preview ();
		}
	}

	enum Effect {
		NONE = 0,
		BLUR,
		PIXELIZE,
		COLOR,
	}

	construct {
		title = _("Censor");
		icon_name = "gth-censor-symbolic";

		action_group = new SimpleActionGroup ();
		// var action = new SimpleAction.stateful ("dither-method", VariantType.STRING, new Variant.string (Dither.Method.ORDERED.to_state ()));
		// action.activate.connect ((_action, param) => {
		// 	var method = param.get_string ();
		// 	action.set_state (method);
		// 	var dither_operation = filter_grid.get_operation (Effect.DITHER) as Dither;
		// 	if (dither_operation != null) {
		// 		dither_operation.method = Dither.Method.from_state (method);
		// 		queue_update_preview ();
		// 	}
		// });
		// action_group.add_action (action);
	}

	Gtk.Builder builder;
	Gth.FilterGrid filter_grid;
	Job thumbnails_job = null;
	Job color_job = null;
	unowned Gtk.Adjustment amount_adjustment;
	ulong amount_changed_id = 0;
	SimpleActionGroup action_group;
	MaskSelector selector;

	const uint THUMBNAIL_SIZE = 140;
}

public class Gth.MaskedFilter : ImageOperation {
	public Graphene.Rect mask;
	public Image filtered;

	public MaskedFilter (ImageOperation _filter) {
		filter = _filter;
		mask = Graphene.Rect.zero ();
		filtered = null;
	}

	public override Gth.Image? execute (Image image, Cancellable cancellable, bool for_preview = false) {
		if (filtered == null) {
			filtered = filter.execute (image, cancellable);
			if (filtered == null) {
				return null;
			}
		}
		var result = image.dup ();
		filtered.copy_pixels_with_mask (result,
			(uint) mask.origin.x,
			(uint) mask.origin.y,
			(uint) mask.size.width,
			(uint) mask.size.height
		);
		return result;
	}

	ImageOperation filter;
}
