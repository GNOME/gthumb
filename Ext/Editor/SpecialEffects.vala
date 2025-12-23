public class Gth.SpecialEffects : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/special-effects.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);

		filter_grid = builder.get_object ("filter_grid") as Gth.FilterGrid;

		/* Translators: this is the name of a filter that produces warmer colors. */
		filter_grid.add (Effect.WARMER, new WarmerColors (), _("Warmer"));

		/* Translators: this is the name of a filter that produces cooler colors. */
		filter_grid.add (Effect.COOLER, new CoolerColors (), _("Cooler"));

		/* Translators: this is the name of an image filter that produces darker edges. */
		filter_grid.add (Effect.VIGNETTE, new Vignette (), _("Darker Edges"));

		/* Translators: this is the name of an image filter that produces blurred edges. */
		filter_grid.add (Effect.BLURRED_EDGES, new BlurredEdges (), _("Blurred Edges"));

		filter_grid.activated.connect ((id) => {
			var operation = filter_grid.get_operation (id) as ParametricOperation;
			if ((operation != null) && operation.has_parameter ()) {
				window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
			}
			else {
				window.editor.set_action_bar (null);
			}
			queue_update_preview ();
		});

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.resized.connect (() => update_preview_on_resize ());
		add_default_controllers (image_view);

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
		// filter_grid.activate (Effect.WARMER);
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
		BLURRED_EDGES;

		public bool has_parameter () {
			return (this != NONE) && (this <= BLURRED_EDGES);
		}
	}

	construct {
		title = _("Special Effects");
		icon_name = "gth-special-effects-symbolic";
	}

	Gtk.Builder builder;
	Gth.FilterGrid filter_grid;
	Job thumbnails_job = null;
	unowned Gtk.Adjustment amount_adjustment;
	ulong amount_changed_id = 0;

	const uint THUMBNAIL_SIZE = 140;
}
