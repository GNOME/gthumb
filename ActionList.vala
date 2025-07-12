public class Gth.ActionList : Gtk.Box {
	construct {
		actions = new GenericList<ActionInfo>();
		var sort_model = new Gtk.SortListModel (actions.model, null);
		sort_model.section_sorter = new Gtk.CustomSorter ((a, b) => {
			unowned var action_a = a as ActionInfo;
			unowned var action_b = b as ActionInfo;
			if ((action_a.category == null) || (action_b.category == null)) {
				return 0;
			}
			return Util.intcmp (action_a.category.sort_order, action_b.category.sort_order);
		});
		var selection_model = new Gtk.NoSelection (sort_model);

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
			activate_action_variant (action.name, action.value);
		});

		var header_factory = new Gtk.SignalListItemFactory ();
		header_factory.setup.connect ((item) => {
			var list_header = item as Gtk.ListHeader;
			var label = new Gtk.Label ("");
			label.halign = Gtk.Align.START;
			label.margin_start = 16;
			label.sensitive = false;
			list_header.child = label;
		});
		header_factory.bind.connect ((item) => {
			var list_header = item as Gtk.ListHeader;
			var label = list_header.child as Gtk.Label;
			var action = list_header.item as ActionInfo;
			if ((action.category != null) && !Strings.empty (action.category.display_name)) {
				label.label = action.category.display_name;
				label.parent.visible = true;
			}
			else {
				label.parent.visible = false;
			}
		});
		view.header_factory = header_factory;

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

public class Gth.ActionInfo : Object {
	public Type type;
	public string name;
	public Variant? value;
	public string display_name;
	public Icon icon;
	public ActionCategory category;

	public ActionInfo (string _name, Variant? _value, string _display_name, Icon? _icon = null) {
		name = _name;
		value = _value;
		display_name = _display_name;
		icon = _icon;
		category = null;
	}

	public ActionInfo.for_file (string action_name, File file, string? _display_name = null) {
		var file_source = app.get_source_for_file (file);
		var info = file_source.get_display_info (file);
		this (action_name,
			new Variant.string (file.get_uri ()),
			(_display_name != null) ? _display_name : info.get_display_name (),
			info.get_symbolic_icon ());
	}

	public string to_string () {
		return "%s [%s(%s)]".printf (display_name, name, (value != null) ? value.print (true) : "");
	}
}

public class Gth.ActionItem : Gtk.Box {
	public ActionItem () {
		spacing = 6;
		orientation = Gtk.Orientation.HORIZONTAL;

		icon = new Gtk.Image ();
		append (icon);

		label = new Gtk.Label ("");
		append (label);
	}

	public void bind (ActionInfo action) {
		if (action.icon != null) {
			icon.set_from_gicon (action.icon);
			icon.visible = true;
			label.set_text (action.display_name);
		}
		else {
			icon.visible = false;
			label.use_underline = true;
			label.set_text_with_mnemonic (action.display_name);
		}
	}

	public void unbind () {
		// void
	}

	Gtk.Image icon;
	Gtk.Label label;
}

public class Gth.ActionCategory {
	public string display_name;
	public int sort_order;

	public ActionCategory (string _display_name, int _sort_order) {
		display_name = _display_name;
		sort_order = _sort_order;
	}
}
