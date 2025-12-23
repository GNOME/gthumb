public class Gth.SpecialEffects : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/special-effects.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);

		filter_grid = builder.get_object ("filter_grid") as Gth.FilterGrid;
		/* Translators: this is the name of a filter that produces warmer colors. */
		filter_grid.add (Effect.WARMER, new ParametricCurveOperation (Effect.WARMER, 0.158730159), _("Warmer"));
		/* Translators: this is the name of a filter that produces cooler colors. */
		filter_grid.add (Effect.COOLER, new ParametricCurveOperation (Effect.COOLER, 0.158730159), _("Cooler"));
		/* Translators: this is the name of an image filter that produces darker edges. */
		filter_grid.add (Effect.VIGNETTE, new VignetteOperation (), _("Darker Edges"));
		/* Translators: this is the name of an image filter that produces blurred edges. */
		filter_grid.add (Effect.BLURRED_EDGES, new BlurredEdgesOperation (), _("Blurred Edges"));
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

	interface ParametricOperation : Object {
		public virtual bool has_parameter () {
			return false;
		}

		public virtual void set_amount (double _amount) {
			// void
		}

		public virtual double get_amount () {
			return 0.0;
		}

		public virtual void reset_amount () {
			// void
		}
	}

	class ParametricCurveOperation : CurveOperation, ParametricOperation {
		public double default_amount;
		public Effect effect;

		public ParametricCurveOperation (Effect _effect, double _amount = 0.0) {
			default_amount = _amount;
			set_effect (_effect, _amount);
		}

		public override bool has_parameter () {
			return effect.has_parameter ();
		}

		public override void set_amount (double _amount) {
			set_effect (effect, _amount);
		}

		public override double get_amount () {
			return amount;
		}

		public override void reset_amount () {
			set_amount (default_amount);
		}

		void set_effect (Effect _effect, double _amount) {
			effect = _effect;
			amount = _amount;
			switch (effect) {
			case Effect.WARMER:
				var red_point = Point.interpolate (Point (127, 127), Point (77, 169), amount);
				var blue_point = Point.interpolate (Point (127, 127), Point (183, 74), amount);
				points = Points () {
					value = null,
					red   = { Point (0, 0), red_point /* Point (117, 136) */, Point (255, 255) },
					green = null,
					blue  = { Point (0, 0), blue_point /* Point (136, 119) */, Point (255, 255) },
				};
				break;

			case Effect.COOLER:
				var red_point = Point.interpolate (Point (127, 127), Point (183, 74), amount);
				var blue_point = Point.interpolate (Point (127, 127), Point (77, 169), amount);
				points = Points () {
					value = null,
					red   = { Point (0, 0), red_point /*Point (136, 119)*/, Point (255, 255) },
					green = null,
					blue  = { Point (0, 0), blue_point /*Point (117, 136)*/, Point (255, 255) },
				};
				break;

			default:
				points = Points ();
				break;
			}
		}

		double amount;
	}

	class VignetteOperation : ImageOperation, ParametricOperation {
		public override bool has_parameter () {
			return true;
		}

		public override void set_amount (double _amount) {
			amount = _amount;
		}

		public override double get_amount () {
			return amount;
		}

		public override void reset_amount () {
			set_amount (0.5);
		}

		public override Gth.Image? execute (Image input, Cancellable cancellable) {
			if (input == null) {
				return null;
			}
			var output = input.dup ();
			if (!output.apply_vignette (amount, cancellable)) {
				return null;
			}
			return output;
		}

		double amount = 0.5;
	}

	class BlurredEdgesOperation : ImageOperation, ParametricOperation {
		public override bool has_parameter () {
			return true;
		}

		public override void set_amount (double _amount) {
			amount = _amount;
		}

		public override double get_amount () {
			return amount;
		}

		public override void reset_amount () {
			set_amount (0.5);
		}

		public override Gth.Image? execute (Image input, Cancellable cancellable) {
			if (input == null) {
				return null;
			}
			var blurred = input.dup ();
			if (!blurred.blur ((int) Math.round (amount * 10), cancellable)) {
				return null;
			}
			var output = input.dup ();
			if (!output.apply_radial_mask (blurred, amount, cancellable)) {
				return null;
			}
			return output;
		}

		double amount = 0.5;
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
