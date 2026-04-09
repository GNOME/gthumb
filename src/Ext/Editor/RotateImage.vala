public class Gth.RotateImage : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/org/gnome/gthumb/ui/rotate-image.ui");
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);
		window.editor.sidebar.insert_action_group ("rotate", action_group);

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.image = original;
		window.editor.set_content (image_view);

		var angle_adjustment = builder.get_object ("angle_adjustment") as Gtk.Adjustment;
		angle_adjustment_id = angle_adjustment.value_changed.connect ((local_adj) => {
			rotator.degrees = (float) local_adj.get_value ();
		});

		var point1_adjustment = builder.get_object ("point1_adjustment") as Gtk.Adjustment;
		point1_adjustment_id = point1_adjustment.value_changed.connect ((local_adj) => {
			rotator.crop_point1 = local_adj.get_value ();
		});

		var reset_button = builder.get_object ("reset_button") as Gtk.Button;
		reset_button.clicked.connect (() => {
			rotator.degrees = 0;
		});

		var align_button = builder.get_object ("align_button") as Adw.ButtonRow;
		align_button.activated.connect (() => {
			var page = builder.get_object ("alignment_page") as Adw.NavigationPage;
			window.editor.sidebar.push (page);
		});

		var background_row = builder.get_object ("background_row") as Adw.ActionRow;
		background_row.activated.connect (() => {
			var local_job = window.new_job (_("Color"));
			var selector = new Gth.ColorSelector ();
			color_job = local_job;
			selector.select_color.begin (window, rotator.background, local_job.cancellable, (_obj, res) => {
				try {
					set_background_color (selector.select_color.end (res));
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

		var alignment_page = builder.get_object ("alignment_page") as Adw.NavigationPage;
		alignment_page.showing.connect (() => {
			image_view.controller = image_alignment;
		});
		alignment_page.hiding.connect (() => {
			image_view.controller = rotator;
		});

		var apply_alignment = builder.get_object ("apply_alignment") as Gtk.Button;
		apply_alignment.clicked.connect (() => {
			var vertical_line = builder.get_object ("vertical_line") as Gtk.CheckButton;
			var orientation = vertical_line.active ? Gtk.Orientation.VERTICAL : Gtk.Orientation.HORIZONTAL;
			rotator.radiants = image_alignment.get_radiants (orientation);
			window.editor.sidebar.pop ();
		});

		rotator = new ImageRotator ();
		rotator.changed_position_parameters.connect (() => {
			var local_adj = builder.get_object ("point1_adjustment") as Gtk.Adjustment;
			SignalHandler.block (local_adj, point1_adjustment_id);
			local_adj.configure (
				rotator.crop_point1,
				rotator.crop_point_min,
				rotator.crop_point_max,
				0.001,
				0.01,
				0);
			SignalHandler.unblock (local_adj, point1_adjustment_id);
		});
		rotator.notify["degrees"].connect (() => {
			var local_adj = builder.get_object ("angle_adjustment") as Gtk.Adjustment;
			SignalHandler.block (local_adj, angle_adjustment_id);
			local_adj.value = rotator.degrees;
			SignalHandler.unblock (local_adj, angle_adjustment_id);
		});

		image_alignment = new ImageAlignment ();

		var rotated_size = settings.get_string (PREF_ROTATE_SIZE);
		action_group.activate_action ("rotated-size", new Variant.string (rotated_size));

		var background_color = Color.get_rgba_from_hexcode (settings.get_string (PREF_ROTATE_BACKGROUND));
		set_background_color (background_color);

		image_view.controller = rotator;
	}

	public override void before_deactivate () {
		if (color_job != null) {
			color_job.cancel ();
		}
		window.editor.sidebar.insert_action_group ("rotate", null);
		image_view.controller = null;
		builder = null;
		settings.set_string (PREF_ROTATE_BACKGROUND, Color.rgba_to_hexcode (rotator.background));
		settings.set_string (PREF_ROTATE_SIZE, rotator.rotated_size.to_state ());
	}

	public override ImageOperation? get_operation () {
		return new RotateOperation (rotator);
	}

	void set_background_color (Gdk.RGBA? color) {
		if (color == null) {
			return;
		}
		var background_preview = builder.get_object ("background_preview") as Gth.ColorPreview;
		background_preview.color = color;
		rotator.background = color;
	}

	construct {
		title = _("Rotate");
		icon_name = "gth-rotate-symbolic";

		action_group = new SimpleActionGroup ();
		var action = new SimpleAction.stateful ("rotated-size", VariantType.STRING, new Variant.string (RotatedSize.ORIGINAL.to_state ()));
		action.activate.connect ((_action, param) => {
			var method = param.get_string ();
			action.set_state (method);
			rotator.rotated_size = RotatedSize.from_state (method);
			var point1_row = builder.get_object ("point1_row") as Gtk.Widget;
			point1_row.sensitive = (rotator.rotated_size == RotatedSize.CROP_BORDERS);
		});
		action_group.add_action (action);

		color_job = null;
		settings = new GLib.Settings (GTHUMB_ROTATE_SCHEMA);
	}

	Gtk.Builder builder;
	SimpleActionGroup action_group;
	ImageRotator rotator;
	ImageAlignment image_alignment;
	ulong angle_adjustment_id;
	ulong point1_adjustment_id;
	Gth.Job color_job;
	GLib.Settings settings;
}

public class Gth.RotateOperation : ImageOperation {
	public RotateOperation (ImageRotator _rotator) {
		rotator = _rotator;
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable, bool for_preview = false) {
		if (input == null) {
			return null;
		}
		return rotator.rotate_image (input, cancellable);
	}

	ImageRotator rotator;
}
