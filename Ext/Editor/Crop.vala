public class Gth.Crop : ImageTool {
	public override void after_activate () {
		builder = new Gtk.Builder.from_resource ("/app/gthumb/gthumb/ui/crop.ui");
		window.editor.set_action_bar (builder.get_object ("action_bar") as Gtk.Widget);

		var options = builder.get_object ("options") as Gtk.Widget;
		options.insert_action_group ("crop", action_group);
		window.editor.set_options (options);

		var more_options = builder.get_object ("more_options") as Gtk.Button;
		more_options.clicked.connect (() => {
			var page = builder.get_object ("more_options_page") as Adw.NavigationPage;
			window.editor.sidebar_view.push (page);
		});

		var entry = builder.get_object ("aspect_ratio_entry") as Gtk.Entry;
		entry.icon_release.connect (() => {
			var local_list = builder.get_object ("aspect_ratio_list") as Gtk.DropDown;
			var local_entry = builder.get_object ("aspect_ratio_entry") as Gtk.Entry;
			local_entry.visible = false;
			local_list.visible = true;
			local_list.selected = 0;
		});
		entry.activate.connect ((obj) => {
			var local_action = action_group.lookup_action ("aspect-ratio") as SimpleAction;
			if (local_action != null) {
				local_action.set_state ("other");
			}
			update_ratio ();
		});

		image_view = builder.get_object ("image_view") as Gth.ImageView;
		image_view.image = original;
		//add_default_controllers (image_view);

		window.editor.set_content (image_view);

		selector = new ImageSelector (image_view);
		selector.changed.connect (() => {
			var spin_row = builder.get_object ("selection_x") as Adw.SpinRow;
			SignalHandler.block (spin_row.adjustment, selection_x_changed_id);
			spin_row.adjustment.freeze_notify ();
			spin_row.adjustment.upper = original.width - selector.selection.size.width;
			spin_row.adjustment.value = selector.selection.origin.x;
			spin_row.adjustment.step_increment = 1;
			spin_row.adjustment.thaw_notify ();
			SignalHandler.unblock (spin_row.adjustment, selection_x_changed_id);

			spin_row = builder.get_object ("selection_y") as Adw.SpinRow;
			SignalHandler.block (spin_row.adjustment, selection_y_changed_id);
			spin_row.adjustment.freeze_notify ();
			spin_row.adjustment.upper = original.height - selector.selection.size.height;
			spin_row.adjustment.value = selector.selection.origin.y;
			spin_row.adjustment.step_increment = 1;
			spin_row.adjustment.thaw_notify ();
			SignalHandler.unblock (spin_row.adjustment, selection_y_changed_id);

			spin_row = builder.get_object ("selection_width") as Adw.SpinRow;
			SignalHandler.block (spin_row.adjustment, selection_width_changed_id);
			spin_row.adjustment.freeze_notify ();
			spin_row.adjustment.upper = original.width - selector.selection.origin.x;
			spin_row.adjustment.value = selector.selection.size.width;
			spin_row.adjustment.step_increment = 1;
			spin_row.adjustment.thaw_notify ();
			SignalHandler.unblock (spin_row.adjustment, selection_width_changed_id);

			spin_row = builder.get_object ("selection_height") as Adw.SpinRow;
			SignalHandler.block (spin_row.adjustment, selection_height_changed_id);
			spin_row.adjustment.freeze_notify ();
			spin_row.adjustment.upper = original.height - selector.selection.origin.y;
			spin_row.adjustment.value = selector.selection.size.height;
			spin_row.adjustment.step_increment = 1;
			spin_row.adjustment.thaw_notify ();
			SignalHandler.unblock (spin_row.adjustment, selection_height_changed_id);
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

		ratios = {};
		uint pos = 0;
		var ratio_list = builder.get_object ("ratio_list") as Gtk.ListBox;

		ratios += 0;
		ratio_list.insert (new_ratio_row (_("Disabled"), pos), (int) pos);
		pos++;

		var display = window.root.get_display ();
		var monitors = display.get_monitors ();
		var n_monitors = monitors.get_n_items ();
		for (var i = 0; i < n_monitors; i++) {
			var monitor = monitors.get_item (i) as Gdk.Monitor;
			var geometry = monitor.geometry;
			var ratio = (float) geometry.width / geometry.height;
			ratios += ratio;
			ratio_list.insert (new_ratio_row (_("Screen"), pos, (n_monitors > 1) ? monitor.model : null), (int) pos);
			pos++;
		}

		ratios += (float) original.width / original.height;
		ratio_list.insert (new_ratio_row (_("Image"), pos), (int) pos);
		pos++;

		ratios += 1;
		ratio_list.insert (new_ratio_row (_("Square"), pos), (int) pos);
		pos++;

		fixed_ratios = pos;

		var ratio_names = new Gtk.StringList (null);
		foreach (var other in OTHER_RATIOS) {
			ratio_names.append ("%d:%d".printf ((int) other.width, (int) other.height));
			ratios += (float) other.width / other.height;
		}

		ratio_names.append (_("Other…"));
		ratios += 0.0f;

		var aspect_ratio_list = builder.get_object ("aspect_ratio_list") as Gtk.DropDown;
		aspect_ratio_list.model = ratio_names;
		aspect_ratio_list.notify["selected"].connect ((obj, param) => {
			var local_action = action_group.lookup_action ("aspect-ratio") as SimpleAction;
			if (local_action != null) {
				local_action.set_state ("other");
			}
			var idx = update_ratio ();
			var local_list = builder.get_object ("aspect_ratio_list") as Gtk.DropDown;
			var local_entry = builder.get_object ("aspect_ratio_entry") as Gtk.Entry;
			if (idx == entry_index ()) {
				if (!local_entry.visible) {
					local_list.visible = false;
					local_entry.visible = true;
					local_entry.grab_focus ();
				}
			}
		});

		var rotated = builder.get_object ("rotated_ratio") as Adw.SwitchRow;
		rotated.notify["active"].connect (() => update_ratio (true));

		var maximize = builder.get_object ("maximize") as Adw.ButtonRow;
		maximize.activated.connect (() => selector.maximize ());

		var center = builder.get_object ("center") as Adw.ButtonRow;
		center.activated.connect (() => selector.center ());

		var step = builder.get_object ("step") as Adw.SpinRow;
		step.adjustment.value_changed.connect ((local_adj) => {
			selector.step = (int) local_adj.value;
		});

		update_ratio ();
		selector.changed ();
	}

	public override void before_deactivate () {
		builder = null;
	}

	public override ImageOperation? get_operation () {
		return new CutOperation (selector.selection);
	}

	Gtk.Widget new_ratio_row (string name, uint idx, string? description = null) {
		var row = new Adw.ActionRow ();
		var check_button = new Gtk.CheckButton ();
		check_button.action_name = "crop.aspect-ratio";
		check_button.action_target= "%u".printf (idx);
		row.add_prefix (check_button);
		if (description != null) {
			var label = new Gtk.Label (description);
			label.add_css_class ("dimmed");
			row.add_suffix (label);
		}
		row.set_title (name);
		row.activatable = true;
		row.activatable_widget = check_button;
		return row;
	}

	uint entry_index () {
		return ratios.length - 1;
	}

	uint update_ratio (bool swap_sides = false) {
		var rotated = builder.get_object ("rotated_ratio") as Adw.SwitchRow;
		var ratio_state = action_group.get_action_state ("aspect-ratio");
		var value = ratio_state.get_string ();

		var local_list = builder.get_object ("aspect_ratio_list") as Gtk.DropDown;
		uint idx = 0;
		if (value == "other") {
			idx = local_list.selected + fixed_ratios;
		}
		else {
			idx = uint.parse (value, 10);
		}

		var local_entry = builder.get_object ("aspect_ratio_entry") as Gtk.Entry;
		var ratio = 0f;
		if (idx == entry_index ()) {
			if (Strings.empty (local_entry.text)) {
				local_entry.text = "%.2f".printf (selector.ratio);
			}
			ratio = float.parse (local_entry.text);
		}
		else {
			ratio = ratios[idx];
		}
		if ((ratio > 0) && rotated.active) {
			ratio = 1f / ratio;
		}
		selector.set_ratio (ratio, swap_sides);
		var step = builder.get_object ("step") as Adw.SpinRow;
		step.sensitive = selector.can_snap ();
		rotated.sensitive = (ratio != 0);
		return idx;
	}

	construct {
		title = _("Crop");
		icon_name = "gth-crop-symbolic";

		action_group = new SimpleActionGroup ();
		var action = new SimpleAction.stateful ("aspect-ratio", VariantType.STRING, new Variant.string ("0"));
		action.activate.connect ((_action, param) => {
			action.set_state (param.get_string ());
			update_ratio ();
		});
		action_group.add_action (action);
	}

	Gtk.Builder builder;
	ImageSelector selector;
	SimpleActionGroup action_group;
	float[] ratios;
	uint fixed_ratios;
	ulong selection_x_changed_id = 0;
	ulong selection_y_changed_id = 0;
	ulong selection_width_changed_id = 0;
	ulong selection_height_changed_id = 0;

	Graphene.Size[] OTHER_RATIOS = {
		{ 16, 9 },
		{ 16, 10 },
		{ 21, 9 },
		{ 5, 4 },
		{ 4, 3 },
		{ 3, 2 },
		{ 2, 1 },
	};
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
