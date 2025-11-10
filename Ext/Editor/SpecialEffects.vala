public class Gth.SpecialEffects : ImageTool {
	public override void activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/special-effects.ui");
		window.editor.sidebar.child = builder.get_object ("options") as Gtk.Widget;

		filter_grid = builder.get_object ("filter_grid") as Gth.FilterGrid;
		/* Translators: this is the name of a filter that produces warmer colors. */
		filter_grid.add (Effect.WARMER, new EffectOperation (Effect.WARMER, 0.158730159), _("Warmer"));
		/* Translators: this is the name of a filter that produces cooler colors. */
		filter_grid.add (Effect.COOLER, new EffectOperation (Effect.COOLER, 0.158730159), _("Cooler"));
		/* Translators: this is the name of an image filter that produces darker edges. */
		//filter_grid.add (Effect.VIGNETTE, new VignetteOperation (), _("Vignette"));
		filter_grid.activated.connect ((id) => {
			var effect = (Effect) id;
			if (effect.has_parameter ()) {
				window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
			}
			else {
				window.editor.set_action_bar (null);
			}
			update_preview (effect);
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
				update_preview ((Effect) filter_grid.get_activated ());
			}
		});

		amount_adjustment = builder.get_object ("amount_adjustment") as Gtk.Adjustment;
		amount_changed_id = amount_adjustment.value_changed.connect (() => {
			var id = filter_grid.get_activated ();
			var operation = filter_grid.get_operation (id) as EffectOperation;
			if (operation != null) {
				operation.set_amount (amount_adjustment.value / 100.0);
				update_preview ((Effect) id);
			}
		});

		unowned var reset_button = builder.get_object ("reset_button") as Gtk.Button;
		reset_button.clicked.connect (() => reset_amount ());

		update_thumbnails ();
		filter_grid.activate (Effect.WARMER);
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

	void update_preview (Effect effect) {
		if (preview_job != null) {
			preview_job.cancel ();
		}
		var job = window.new_job ("Update Preview");
		preview_job = job;
		preview_filter_async.begin (effect, job.cancellable, (_obj, res) => {
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

	async Image? preview_filter_async (Effect effect, Cancellable cancellable) throws Error {
		if (resized == null) {
			resized = original; //yield original.resize_async (get_preview_size (), ResizeFlags.DEFAULT, ScaleFilter.BOX, cancellable);
		}
		var operation = filter_grid.get_operation ((int) effect) as EffectOperation;
		if (operation == null) {
			return resized;
		}
		SignalHandler.block (amount_adjustment, amount_changed_id);
		amount_adjustment.set_value (Math.round (operation.amount * 100.0));
		SignalHandler.unblock (amount_adjustment, amount_changed_id);
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
		var operation = filter_grid.get_operation (id) as EffectOperation;
		if (operation != null) {
			operation.set_amount (operation.default_amount);
		}
		update_preview ((Effect) id);
	}

	enum Effect {
		NONE = 0,
		WARMER,
		COOLER,
		// No parameters:
		VIGNETTE;

		public bool has_parameter () {
			return (this != NONE) && (this <= COOLER);
		}
	}

	class EffectOperation : CurveOperation {
		public double amount;
		public double default_amount;
		public Effect effect;

		public EffectOperation (Effect _effect, double _amount = 0.0) {
			default_amount = _amount;
			set_effect (_effect, _amount);
		}

		public void set_amount (double _amount) {
			set_effect (effect, _amount);
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
	}

	class VignetteOperation : ImageOperation {
		public override Gth.Image? execute (Image input, Cancellable cancellable) {
			if (input == null) {
				return null;
			}
			var output = input.dup ();
			// output.apply_vignette (value_map, 127); // TODO: make parametric
			return output;
		}
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
	unowned Gtk.Adjustment amount_adjustment;
	Image original;
	Image resized;
	ulong amount_changed_id = 0;

	const uint THUMBNAIL_SIZE = 140;
}
