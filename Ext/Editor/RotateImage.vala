public class Gth.RotateImage : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/rotate-image.ui");
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

		rotator = new ImageRotator (image_view);
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
		image_view.controller = rotator;
	}

	public override void before_deactivate () {
		window.editor.sidebar.insert_action_group ("rotate", null);
		builder = null;
	}

	public override ImageOperation? get_operation () {
		return new RotateOperation (rotator);
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
	}

	Gtk.Builder builder;
	SimpleActionGroup action_group;
	ImageRotator rotator;
	ulong angle_adjustment_id;
	ulong point1_adjustment_id;
}

public class Gth.RotateOperation : ImageOperation {
	public RotateOperation (ImageRotator _rotator) {
		rotator = _rotator;
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable) {
		if (input == null) {
			return null;
		}
		return rotator.rotate_image (input, cancellable);
	}

	ImageRotator rotator;
}
