[GtkTemplate (ui = "/app/gthumb/gthumb/ui/grid-group.ui")]
public class Gth.GridGroup : Adw.PreferencesGroup {
	public signal void changed ();

	public GridType grid_type {
		get {
			var state = action_group.get_action_state ("grid-type");
			return GridType.from_state (state.get_string ());
		}
		set {
			Util.set_state (action_group, "grid-type", new Variant.string (value.to_state ()));
		}
	}

	Gtk.Widget new_ratio_row (string name, uint idx, string? description = null) {
		var row = new Adw.ActionRow ();
		var check_button = new Gtk.CheckButton ();
		check_button.action_name = "group.aspect-ratio";
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

	construct {
		action_group = new SimpleActionGroup ();
		var action = new SimpleAction.stateful ("grid-type", VariantType.STRING, new Variant.string (GridType.NONE.to_state ()));
		action.activate.connect ((_action, param) => {
			// unowned var state = param.get_string ();
			// _action.set_state (state);
			grid_type = GridType.from_state (param.get_string ());
		});
		action_group.add_action (action);
		insert_action_group ("grid_group", action_group);

		grid_list.append (new_grid_type_row (GridType.NONE, _("None")));
		grid_list.append (new_grid_type_row (GridType.RULE_OF_THIRDS, _("Rule of Thirds")));
		grid_list.append (new_grid_type_row (GridType.GOLDEN_RATIO, _("Golden Ratio")));
		grid_list.append (new_grid_type_row (GridType.CENTER_LINES, _("Center Lines")));
		grid_list.append (new_grid_type_row (GridType.UNIFORM, _("Uniform")));
	}

	Gtk.Widget new_grid_type_row (GridType _grid_type, string name) {
		var row = new Adw.ActionRow ();
		var check_button = new Gtk.CheckButton ();
		check_button.action_name = "grid_group.grid-type";
		check_button.action_target = _grid_type.to_state ();
		row.add_prefix (check_button);
		row.set_title (name);
		row.activatable = true;
		row.activatable_widget = check_button;
		return row;
	}

	SimpleActionGroup action_group;
	[GtkChild] Gtk.ListBox grid_list;
}
