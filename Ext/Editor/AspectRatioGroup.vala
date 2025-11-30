[GtkTemplate (ui = "/app/gthumb/gthumb/ui/aspect-ratio-group.ui")]
public class Gth.AspectRatioGroup : Adw.PreferencesGroup {
	public signal void changed (bool after_rotation);

	public float ratio { get; set; default = 1; }

	public void activate (Gtk.Window window, Gth.Image original, bool activate_image_ratio = false) {
		entry.icon_release.connect (() => {
			entry.visible = false;
			other_list.visible = true;
			other_list.selected = 0;
		});
		entry.activate.connect ((obj) => {
			var local_action = action_group.lookup_action ("aspect-ratio") as SimpleAction;
			if (local_action != null) {
				local_action.set_state ("other");
			}
			update_ratio ();
		});

		ratios = {};
		uint pos = 0;

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
			var description = "%u:%u".printf ((uint) geometry.width, (uint) geometry.height);
			ratio_list.insert (new_ratio_row (_("Screen"), pos, description), (int) pos);
			pos++;
		}

		ratios += (float) original.width / original.height;
		ratio_list.insert (new_ratio_row (_("Image"), pos, "%u:%u".printf (original.width, original.height)), (int) pos);
		image_pos = pos;
		pos++;

		ratios += 1;
		ratio_list.insert (new_ratio_row (_("Square"), pos, "1:1"), (int) pos);
		pos++;

		fixed_ratios = pos;

		var ratio_names = new Gtk.StringList (null);
		foreach (var other in OTHER_RATIOS) {
			ratio_names.append ("%d:%d".printf ((int) other.width, (int) other.height));
			ratios += (float) other.width / other.height;
		}

		ratio_names.append (_("Other…"));
		ratios += 0.0f;

		other_list.model = ratio_names;
		other_list.notify["selected"].connect ((obj, param) => {
			var local_action = action_group.lookup_action ("aspect-ratio") as SimpleAction;
			if (local_action != null) {
				local_action.set_state ("other");
			}
			var idx = update_ratio ();
			if (idx == entry_index ()) {
				if (!entry.visible) {
					other_list.visible = false;
					entry.visible = true;
					entry.grab_focus ();
				}
			}
		});

		rotated.notify["active"].connect (() => update_ratio (true));
		rotated.sensitive = false;

		if (activate_image_ratio) {
			set_image_ratio ();
		}
	}

	public void disable_ratio () {
		Util.set_state (action_group, "aspect-ratio", new Variant.string ("%d".printf (0)));
	}

	public void set_image_ratio () {
		Util.set_state (action_group, "aspect-ratio", new Variant.string ("%u".printf (image_pos)));
	}

	uint entry_index () {
		return ratios.length - 1;
	}

	Gtk.Widget new_ratio_row (string name, uint idx, string? description = null) {
		var row = new Adw.ActionRow ();
		var check_button = new Gtk.CheckButton ();
		check_button.action_name = "aspect_ratio_group.aspect-ratio";
		check_button.action_target = "%u".printf (idx);
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

	uint update_ratio (bool after_rotation = false) {
		var ratio_state = action_group.get_action_state ("aspect-ratio");
		var value = ratio_state.get_string ();

		uint idx = 0;
		if (value == "other") {
			idx = other_list.selected + fixed_ratios;
		}
		else {
			idx = uint.parse (value, 10);
		}

		var new_ratio = 0f;
		if (idx == entry_index ()) {
			if (Strings.empty (entry.text)) {
				entry.text = "%.2f".printf (ratio);
			}
			new_ratio = float.parse (entry.text);
		}
		else {
			new_ratio = ratios[idx];
		}
		if ((new_ratio > 0) && rotated.active) {
			new_ratio = 1f / new_ratio;
		}
		rotated.sensitive = (new_ratio != 0);
		ratio = new_ratio;
		changed (after_rotation);
		return idx;
	}

	construct {
		action_group = new SimpleActionGroup ();
		var action = new SimpleAction.stateful ("aspect-ratio", VariantType.STRING, new Variant.string ("0"));
		action.activate.connect ((_action, param) => {
			_action.set_state (param.get_string ());
			update_ratio ();
		});
		action_group.add_action (action);

		insert_action_group ("aspect_ratio_group", action_group);
	}

	SimpleActionGroup action_group;
	float[] ratios;
	uint fixed_ratios;
	uint image_pos;
	[GtkChild] Gtk.ListBox ratio_list;
	[GtkChild] Gtk.Entry entry;
	[GtkChild] Adw.SwitchRow rotated;
	[GtkChild] Gtk.DropDown other_list;
	[GtkChild] Gtk.CheckButton other;

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
