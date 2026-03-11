[GtkTemplate (ui = "/app/gthumb/gthumb/ui/aspect-ratio-group.ui")]
public class Gth.AspectRatioGroup : Adw.PreferencesGroup {
	public signal void changed (bool after_rotation);

	public float ratio { get; set; default = 0; }

	public new string description { get { return get_ratio_description (); } }

	public bool show_title {
		set {
			title = value ? _("Aspect Ratio") : "";
		}
	}

	public float custom_ratio {
		get {
			return float.parse (entry.text);
		}
		set {
			entry.text = RATIO_FORMAT.printf (value);
		}
	}

	public new void activate (Gtk.Window window, Gth.Image original, bool activate_image_ratio = false) {
		entry.icon_release.connect (() => {
			entry.visible = false;
			entry.text = "";
			other_list.visible = true;
			other_list.selected = 0;
		});
		entry.activate.connect ((obj) => {
			set_other_ratio ();
			update_ratio ();
		});

		ratios = {};
		descriptions = {};
		uint pos = 0;

		ratios += 0;
		descriptions += C_("Aspect Ratio", "Disabled");
		ratio_list.insert (new_ratio_row (C_("Aspect Ratio", "Disabled"), pos), (int) pos);
		pos++;

		var display = window.root.get_display ();
		var monitors = display.get_monitors ();
		var n_monitors = monitors.get_n_items ();
		for (var i = 0; i < n_monitors; i++) {
			var monitor = monitors.get_item (i) as Gdk.Monitor;
			var geometry = monitor.geometry;
			ratios += (float) geometry.width / geometry.height;
			descriptions += C_("Aspect Ratio", "Screen");
			var _description = "%u:%u".printf ((uint) geometry.width, (uint) geometry.height);
			ratio_list.insert (new_ratio_row (C_("Aspect Ratio", "Screen"), pos, _description), (int) pos);
			pos++;
		}

		ratios += (float) original.width / original.height;
		descriptions += C_("Aspect Ratio", "Image");
		ratio_list.insert (new_ratio_row (C_("Aspect Ratio", "Image"), pos, "%u:%u".printf (original.width, original.height)), (int) pos);
		image_pos = pos;
		pos++;

		ratios += 1;
		descriptions += C_("Aspect Ratio", "Square");
		ratio_list.insert (new_ratio_row (C_("Aspect Ratio", "Square"), pos, "1:1"), (int) pos);
		pos++;

		fixed_ratios = pos;

		var ratio_names = new Gtk.StringList (null);
		foreach (var other in OTHER_RATIOS) {
			var name = "%d:%d".printf ((int) other.width, (int) other.height);
			ratio_names.append (name);
			ratios += (float) (other.width / other.height);
			descriptions += name;
		}

		ratio_names.append (C_("Aspect Ratio", "A4"));
		ratios += (float) (1.0 / Math.sqrt (2));
		descriptions += C_("Aspect Ratio", "A4");

		ratio_names.append (_("Other…"));
		ratios += 0.0f;

		other_list.model = ratio_names;
		float entry_ratio = float.parse (entry.text);
		if (entry_ratio != 0) {
			// Select "Other"
			other_list.selected = entry_index () - fixed_ratios;
			other_list.visible = false;
			entry.visible = true;
		}
		other_list.notify["selected"].connect ((obj, param) => {
			set_other_ratio ();
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
		action_group.activate_action ("aspect-ratio", new Variant.string ("0"));
	}

	public void set_image_ratio () {
		rotated.active = false;
		action_group.activate_action ("aspect-ratio", new Variant.string ("%u".printf (image_pos)));
	}

	uint entry_index () {
		return ratios.length - 1;
	}

	Gtk.Widget new_ratio_row (string name, uint idx, string? _description = null) {
		var row = new Adw.ActionRow ();
		var check_button = new Gtk.CheckButton ();
		check_button.action_name = "aspect-ratio-group.aspect-ratio";
		check_button.action_target = "%u".printf (idx);
		row.add_prefix (check_button);
		if (_description != null) {
			var label = new Gtk.Label (_description);
			label.add_css_class ("dimmed");
			row.add_suffix (label);
		}
		row.set_title (name);
		row.activatable = true;
		row.activatable_widget = check_button;
		return row;
	}

	uint get_selected_ratio_index () {
		var ratio_state = action_group.get_action_state ("aspect-ratio");
		var value = ratio_state.get_string ();
		var idx = 0u;
		if (value == "other") {
			idx = other_list.selected + fixed_ratios;
		}
		else {
			idx = uint.parse (value, 10);
		}
		return idx;
	}

	uint update_ratio (bool after_rotation = false) {
		var new_ratio = 0f;
		var idx = get_selected_ratio_index ();
		if (idx == entry_index ()) {
			if (Strings.empty (entry.text)) {
				entry.text = RATIO_FORMAT.printf (ratio);
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
		_description = null;
		changed (after_rotation);
		return idx;
	}

	unowned string get_ratio_description () {
		if (_description == null) {
			var idx = get_selected_ratio_index ();
			if (idx == entry_index ()) {
				_description = RATIO_FORMAT.printf (ratio);
			}
			else {
				_description = descriptions[idx];
				if (rotated.active) {
					if (idx >= fixed_ratios) {
						var other = OTHER_RATIOS[idx - fixed_ratios];
						_description = "%d:%d".printf ((int) other.height, (int) other.width);
					}
					else {
						_description = _("%s (Rotated)").printf (descriptions[idx]);
					}
				}
				else {
					_description = descriptions[idx];
				}
			}
		}
		return _description;
	}

	void set_other_ratio () {
		var local_action = action_group.lookup_action ("aspect-ratio") as SimpleAction;
		if (local_action != null) {
			local_action.set_state ("other");
		}
	}

	construct {
		action_group = new SimpleActionGroup ();

		var action = new SimpleAction.stateful ("aspect-ratio", VariantType.STRING, new Variant.string ("0"));
		action.activate.connect ((_action, param) => {
			_action.set_state (param.get_string ());
			update_ratio ();
		});
		action_group.add_action (action);

		insert_action_group ("aspect-ratio-group", action_group);
	}

	SimpleActionGroup action_group;
	float[] ratios;
	string[] descriptions;
	uint fixed_ratios;
	uint image_pos;
	string _description = null;
	[GtkChild] unowned Gtk.ListBox ratio_list;
	[GtkChild] unowned Gtk.Entry entry;
	[GtkChild] unowned Adw.SwitchRow rotated;
	[GtkChild] unowned Gtk.DropDown other_list;
	const string RATIO_FORMAT = "%.3f";
}
