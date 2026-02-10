public class Gth.Crop : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/crop.ui");
		window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);
		window.editor.set_options (builder.get_object ("options") as Gtk.Widget);

		var more_options = builder.get_object ("more_options") as Gtk.Button;
		more_options.clicked.connect (() => {
			var page = builder.get_object ("more_options_page") as Adw.NavigationPage;
			window.editor.sidebar.push (page);
		});

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.image = original;
		window.editor.set_content (image_view);

		selector = new ImageSelector ();
		selector.changed.connect (() => {
			var spin_row = builder.get_object ("selection_x") as Adw.SpinRow;
			if (selection_x_changed_id != 0) {
				SignalHandler.block (spin_row.adjustment, selection_x_changed_id);
			}
			spin_row.adjustment.freeze_notify ();
			spin_row.adjustment.upper = original.width - selector.selection.size.width;
			spin_row.adjustment.value = selector.selection.origin.x;
			spin_row.adjustment.step_increment = 1;
			spin_row.adjustment.thaw_notify ();
			if (selection_x_changed_id != 0) {
				SignalHandler.unblock (spin_row.adjustment, selection_x_changed_id);
			}

			spin_row = builder.get_object ("selection_y") as Adw.SpinRow;
			if (selection_y_changed_id != 0) {
				SignalHandler.block (spin_row.adjustment, selection_y_changed_id);
			}
			spin_row.adjustment.freeze_notify ();
			spin_row.adjustment.upper = original.height - selector.selection.size.height;
			spin_row.adjustment.value = selector.selection.origin.y;
			spin_row.adjustment.step_increment = 1;
			spin_row.adjustment.thaw_notify ();
			if (selection_y_changed_id != 0) {
				SignalHandler.unblock (spin_row.adjustment, selection_y_changed_id);
			}

			spin_row = builder.get_object ("selection_width") as Adw.SpinRow;
			if (selection_width_changed_id != 0) {
				SignalHandler.block (spin_row.adjustment, selection_width_changed_id);
			}
			spin_row.adjustment.freeze_notify ();
			spin_row.adjustment.upper = original.width - selector.selection.origin.x;
			spin_row.adjustment.value = selector.selection.size.width;
			spin_row.adjustment.step_increment = 1;
			spin_row.adjustment.thaw_notify ();
			if (selection_width_changed_id != 0) {
				SignalHandler.unblock (spin_row.adjustment, selection_width_changed_id);
			}

			spin_row = builder.get_object ("selection_height") as Adw.SpinRow;
			if (selection_height_changed_id != 0) {
				SignalHandler.block (spin_row.adjustment, selection_height_changed_id);
			}
			spin_row.adjustment.freeze_notify ();
			spin_row.adjustment.upper = original.height - selector.selection.origin.y;
			spin_row.adjustment.value = selector.selection.size.height;
			spin_row.adjustment.step_increment = 1;
			spin_row.adjustment.thaw_notify ();
			if (selection_height_changed_id != 0) {
				SignalHandler.unblock (spin_row.adjustment, selection_height_changed_id);
			}
		});
		image_view.controller = selector;

		var spin_row = builder.get_object ("selection_x") as Adw.SpinRow;
		selection_x_changed_id = spin_row.adjustment.value_changed.connect ((obj) => {
			var local_adj = obj as Gtk.Adjustment;
			selector.set_x ((float) local_adj.value);
		});

		spin_row = builder.get_object ("selection_y") as Adw.SpinRow;
		selection_y_changed_id = spin_row.adjustment.value_changed.connect ((obj) => {
			var local_adj = obj as Gtk.Adjustment;
			selector.set_y ((float) local_adj.value);
		});

		spin_row = builder.get_object ("selection_width") as Adw.SpinRow;
		selection_width_changed_id = spin_row.adjustment.value_changed.connect ((obj) => {
			var local_adj = obj as Gtk.Adjustment;
			selector.set_width ((float) local_adj.value);
		});

		spin_row = builder.get_object ("selection_height") as Adw.SpinRow;
		selection_height_changed_id = spin_row.adjustment.value_changed.connect ((obj) => {
			var local_adj = obj as Gtk.Adjustment;
			selector.set_height ((float) local_adj.value);
		});

		var aspect_ratio_group = builder.get_object ("aspect_ratio_group") as Gth.AspectRatioGroup;
		aspect_ratio_group.custom_ratio = (float) settings.get_double (PREF_CROP_CUSTOM_RATIO);
		aspect_ratio_group.activate (window, original);
		aspect_ratio_group.changed.connect ((local_group, after_rotation) => {
			update_ratio (after_rotation);
		});

		var maximize = builder.get_object ("maximize") as Adw.ButtonRow;
		maximize.activated.connect (() => selector.maximize ());

		var center = builder.get_object ("center") as Adw.ButtonRow;
		center.activated.connect (() => selector.center ());

		var step = builder.get_object ("step") as Adw.SpinRow;
		step.adjustment.value_changed.connect ((local_adj) => {
			selector.step = (int) local_adj.value;
		});

		var grid_group = builder.get_object ("grid_group") as Gth.GridGroup;
		grid_group.notify["grid-type"].connect ((obj, _param) => {
			var local_grid = obj as Gth.GridGroup;
			selector.grid_type = local_grid.grid_type;
		});
		grid_group.grid_type = Gth.GridType.RULE_OF_THIRDS;

		update_ratio ();
		selector.changed ();
	}

	public override void before_deactivate () {
		var aspect_ratio_group = builder.get_object ("aspect_ratio_group") as Gth.AspectRatioGroup;
		settings.set_double (PREF_CROP_CUSTOM_RATIO, aspect_ratio_group.custom_ratio);
		window.editor.sidebar.insert_action_group ("crop", null);
		image_view.controller = null;
		builder = null;
	}

	public override ImageOperation? get_operation () {
		return new CutOperation (selector.selection);
	}

	void update_ratio (bool swap_sides = false) {
		var aspect_ratio_group = builder.get_object ("aspect_ratio_group") as Gth.AspectRatioGroup;
		selector.set_ratio (aspect_ratio_group.ratio, swap_sides);
		var step = builder.get_object ("step") as Adw.SpinRow;
		step.sensitive = selector.can_snap ();
	}

	construct {
		title = _("Crop");
		icon_name = "gth-crop-symbolic";
		settings = new GLib.Settings (GTHUMB_CROP_SCHEMA);
	}

	GLib.Settings settings;
	Gtk.Builder builder;
	ImageSelector selector;
	ulong selection_x_changed_id = 0;
	ulong selection_y_changed_id = 0;
	ulong selection_width_changed_id = 0;
	ulong selection_height_changed_id = 0;
}

public class Gth.CutOperation : ImageOperation {
	public CutOperation (Graphene.Rect _selection) {
		selection = _selection;
	}

	public override Gth.Image? execute (Image input, Cancellable cancellable) {
		if (input == null) {
			return null;
		}
		var output = input.cut ((uint) selection.origin.x, (uint) selection.origin.y,
			(uint) selection.size.width, (uint) selection.size.height, cancellable);
		return output;
	}

	public Graphene.Rect selection;
}
