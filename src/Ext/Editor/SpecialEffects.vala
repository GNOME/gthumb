public class Gth.SpecialEffects : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/org/gnome/gthumb/ui/special-effects.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);
		window.editor.sidebar.insert_action_group ("special-effects", action_group);

		filter_grid = builder.get_object ("filter_grid") as Gth.FilterGrid;

		// Translators: this is the name of a filter that produces warmer colors.
		filter_grid.add (Effect.WARMER, new WarmerColors (), _("Warmer"));

		// Translators: this is the name of a filter that produces cooler colors.
		filter_grid.add (Effect.COOLER, new CoolerColors (), _("Cooler"));

		// Translators: this is the name of an image filter that produces darker edges.
		filter_grid.add (Effect.VIGNETTE, new Vignette (), _("Darker Edges"));

		// Translators: this is the name of an image filter that produces blurred edges.
		filter_grid.add (Effect.BLURRED_EDGES, new BlurredEdges (), _("Blurred Edges"));

		// Translators: this is the name of an image filter.
		filter_grid.add (Effect.VINTAGE, new Vintage (), _("Vintage"));

		// Translators: this is the name of an image filter.
		filter_grid.add (Effect.LOMO, new Lomo (), _("Lomography"));

		// Translators: this is the name of an image filter.
		filter_grid.add (Effect.DITHER, new Dither (), _("8 Colors"));

		// Translators: this is the name of an image filter.
		filter_grid.add (Effect.NEGATIVE, new NegativeColors (), _("Negative"));

		filter_grid.activated.connect ((id) => {
			var method = (Effect) id;
			var operation = filter_grid.get_operation (method) as ParametricOperation;
			if ((operation != null) && operation.has_parameter ()) {
				window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
			}
			else if (method == Effect.DITHER) {
				window.editor.set_action_bar (builder.get_object ("dither_action_bar") as Gtk.Widget);
			}
			else {
				window.editor.set_action_bar (null);
			}
			queue_update_preview ();
		});

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		add_default_controllers (image_view);
		image_view.zoom_limit = ZoomLimit.MAXIMIZE;
		image_view.default_zoom_type = ZoomType.MAXIMIZE_IF_LARGER;
		image_view.image = viewer.image_view.image;

		window.editor.set_content (image_view);

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
	}

	public override void before_deactivate () {
		window.editor.sidebar.insert_action_group ("special-effects", null);
		if (thumbnails_job != null) {
			thumbnails_job.cancel ();
		}
		builder = null;
	}

	public override ImageOperation? get_operation () {
		return filter_grid.get_active_operation ();
	}

	public override void update_options_from_operation (ImageOperation image_operation) {
		var operation = image_operation as ParametricOperation;
		SignalHandler.block (amount_adjustment, amount_changed_id);
		amount_adjustment.set_value (Math.round (operation.get_amount () * 100.0));
		SignalHandler.unblock (amount_adjustment, amount_changed_id);
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
		WARMER,
		COOLER,
		VIGNETTE,
		BLURRED_EDGES,
		VINTAGE,
		LOMO,
		DITHER,
		NEGATIVE,
	}

	construct {
		title = _("Special Effects");
		icon_name = "gth-special-effects-symbolic";

		action_group = new SimpleActionGroup ();
		var action = new SimpleAction.stateful ("dither-method", VariantType.STRING, new Variant.string (Dither.Method.ORDERED.to_state ()));
		action.activate.connect ((_action, param) => {
			var method = param.get_string ();
			action.set_state (method);
			var dither_operation = filter_grid.get_operation (Effect.DITHER) as Dither;
			if (dither_operation != null) {
				dither_operation.method = Dither.Method.from_state (method);
				queue_update_preview ();
			}
		});
		action_group.add_action (action);
	}

	Gtk.Builder builder;
	Gth.FilterGrid filter_grid;
	Job thumbnails_job = null;
	unowned Gtk.Adjustment amount_adjustment;
	ulong amount_changed_id = 0;
	SimpleActionGroup action_group;

	const uint THUMBNAIL_SIZE = 140;
}
