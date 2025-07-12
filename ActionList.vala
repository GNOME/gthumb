public class Gth.ActionList : Gtk.Box {
	construct {
		actions = new GenericList<ActionInfo>();
		var selection_model = new Gtk.NoSelection (actions.model);
		var factory = new Gtk.SignalListItemFactory ();
		factory.setup.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			list_item.child = new ActionItem ();
		});
		factory.teardown.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			list_item.child = null;
		});
		factory.bind.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			var action_item = list_item.child as ActionItem;
			var action_info = list_item.item as ActionInfo;
			action_item.bind (action_info);
		});
		factory.unbind.connect ((obj) => {
			var list_item = obj as Gtk.ListItem;
			var action_item = list_item.child as ActionItem;
			if (action_item != null) {
				action_item.unbind ();
			}
		});
		view = new Gtk.ListView (selection_model, factory);
		view.add_css_class ("navigation-sidebar");
		view.add_css_class ("action-list");
		view.set_single_click_activate (true);
		view.activate.connect ((pos) => {
			var popover = get_popover ();
			if (popover != null) {
				popover.popdown ();
			}
			var action = actions.model.get_item (pos) as ActionInfo;
			//stdout.printf (">> %s\n", action.to_string ());
			activate_action_variant (action.name, action.value);
		});
		append (view);
	}

	public Gtk.Popover? get_popover () {
		unowned var p = parent;
		while ((p != null) && !(p is Gtk.Popover)) {
			p = p.parent;
		}
		return (p != null) ? p as Gtk.Popover : null;
	}

	public void append_action (ActionInfo action) {
		actions.model.append (action);
	}

	public void remove_all_actions () {
		actions.model.remove_all ();
	}

	Gtk.ListView view;
	GenericList<ActionInfo> actions;
}

public class ActionInfo : Object {
	public string name;
	public Variant? value;
	public string display_name;
	public Icon icon;

	public ActionInfo (string _name, Variant? _value, string _display_name, Icon _icon) {
		name = _name;
		value = _value;
		display_name = _display_name;
		icon = _icon;
	}

	public ActionInfo.for_file (string action_name, File file) {
		var file_source = app.get_source_for_file (file);
		var info = file_source.get_display_info (file);
		this (action_name,
			new Variant.string (file.get_uri ()),
			info.get_display_name (),
			info.get_symbolic_icon ());
	}

	public string to_string () {
		return "%s [%s(%s)]".printf (display_name, name, (value != null) ? value.print (true) : "");
	}
}

public class ActionItem : Gtk.Box {
	public ActionItem () {
		spacing = 6;

		icon = new Gtk.Image ();
		append (icon);

		label = new Gtk.Label ("");
		append (label);
	}

	public void bind (ActionInfo action) {
		icon.set_from_gicon (action.icon);
		label.set_text (action.display_name);
	}

	public void unbind () {
		// void
	}

	Gtk.Image icon;
	Gtk.Label label;
}
