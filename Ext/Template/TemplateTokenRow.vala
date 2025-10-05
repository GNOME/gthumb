[GtkTemplate (ui = "/app/gthumb/gthumb/ui/template-token-row.ui")]
public class Gth.TemplateTokenRow : Gtk.ListBoxRow {
	public TemplateToken token;

	public signal void move_to_row (TemplateTokenRow row);
	public signal void move_to_top ();
	public signal void move_to_bottom ();
	public signal void delete_row ();
	public signal void add_row ();
	public signal void edit_entry (string title, Gtk.Entry entry);
	public signal void data_changed ();

	public TemplateTokenRow (TemplateToken _token, bool as_icon_content = false) {
		token = _token;
		title.label = _(token.code.description);
		var args = Template.get_token_args (token.value);
		switch (token.code.type) {
		case TemplateCodeType.ASK_VALUE:
			if ((args != null) && (args[0] != null)) {
				prompt.text = args[0];
			}
			if ((args != null) && (args[1] != null)) {
				default_value.text = args[1];
			}
			extra_content.visible = true;
			break;

		case TemplateCodeType.TEXT:
			value.text = token.value;
			edit_value_box.visible = true;
			break;

		case TemplateCodeType.QUOTED:
			if ((args != null) && (args[0] != null)) {
				value.text = args[0];
			}
			value.visible = true;
			edit_value_box.visible = true;
			edit_value_button.visible = true;
			break;

		case TemplateCodeType.DATE:
			var selected_format = ((args != null) && (args[0] != null)) ? args[0] : "";
			value.text = selected_format;
			var date_formats = new Gtk.StringList (null);
			var now = new GLib.DateTime.now_local ();
			var selected = -1;
			var idx = 0;
			foreach (unowned var format in DATE_FORMATS) {
				date_formats.append (now.format (format));
				if ((selected == -1) && (format == selected_format)) {
					selected = idx;
				}
				idx++;
			}
			date_formats.append (_("Other…"));
			if (selected == -1) {
				if (!Strings.empty (selected_format)) {
					selected = (int) date_formats.get_n_items () - 1;
				}
				else {
					selected = 0;
				}
			}
			date_format.model = date_formats;
			date_format.visible = true;
			date_format.notify["selected"].connect (() => {
				if (date_format.selected == DATE_FORMATS.length) {
					date_format.visible = false;
					edit_value_box.visible = true;
					edit_value_button.visible = true;
					hide_value_button.visible = true;
				}
			});
			date_format.selected = selected;
			break;

		case TemplateCodeType.FILE_ATTRIBUTE:
			attribute_selector.expression = new Gtk.PropertyExpression (typeof (AttributeItem), null, "title");

			var header_factory = new Gtk.SignalListItemFactory ();
			header_factory.setup.connect ((item) => {
				var list_header = item as Gtk.ListHeader;
				var label = new Gtk.Label ("");
				label.add_css_class ("selector-header");
				label.halign = Gtk.Align.START;
				list_header.child = label;
			});
			header_factory.bind.connect ((item) => {
				var list_header = item as Gtk.ListHeader;
				var attribute = list_header.item as AttributeItem;
				var label = list_header.child as Gtk.Label;
				if (attribute.category != null) {
					label.label = _(attribute.category.display_name);
				}
				else {
					label.label = "";
				}
			});
			attribute_selector.header_factory = header_factory;

			unowned var all_metadata = MetadataInfo.get_all ();
			attributes = new GenericList<AttributeItem> ();
			foreach (unowned var info in all_metadata) {
				if (!(MetadataFlags.HIDDEN in info.flags)) {
					attributes.model.append (new AttributeItem (info));
				}
			}

			var sort_model = new Gtk.SortListModel (attributes.model, null);
			sort_model.section_sorter = new Gtk.CustomSorter ((a, b) => {
				unowned var attr_a = a as AttributeItem;
				unowned var attr_b = b as AttributeItem;
				return Util.intcmp (attr_a.category.sort_order, attr_b.category.sort_order);
			});

			attribute_selector.model = sort_model;
			attribute_selector.visible = true;
			break;

		default:
			break;
		}
		if (as_icon_content) {
			return;
		}
		var drag_source = new Gtk.DragSource ();
		drag_source.set_actions (Gdk.DragAction.MOVE);
		drag_source.prepare.connect ((_source, x, y) => {
			hot_x = (int) x;
			hot_y = (int) y;
			return new Gdk.ContentProvider.for_value (this);
		});
		drag_source.drag_begin.connect ((_obj, drag) => {
			var icon_widget = new Gtk.ListBox ();
			icon_widget.set_size_request (this.get_width (), this.get_height ());
			icon_widget.append (new TemplateTokenRow (token, true));

			unowned var drag_icon = Gtk.DragIcon.get_for_drag (drag) as Gtk.DragIcon;
			drag_icon.set_child (icon_widget);
			drag.set_hotspot (hot_x, hot_y);
		});
		add_controller (drag_source);

		var drop_target = new Gtk.DropTarget (typeof (TemplateTokenRow), Gdk.DragAction.MOVE);
		drop_target.drop.connect ((target, value, x, y) => {
			var source = value.get_object () as TemplateTokenRow;
			if ((source == null) || (source == this) || (source.parent != this.parent)) {
				return false;
			}
			source.move_to_row (this);
			return true;
		});
		add_controller (drop_target);

		var action_group = new SimpleActionGroup ();
		insert_action_group ("row", action_group);

		var action = new SimpleAction ("move-to-top", null);
		action.activate.connect ((_action, param) => {
			move_to_top ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("move-to-bottom", null);
		action.activate.connect ((_action, param) => {
			move_to_bottom ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("delete", null);
		action.activate.connect ((_action, param) => {
			delete_row ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("add", null);
		action.activate.connect ((_action, param) => {
			add_row ();
		});
		action_group.add_action (action);

		action = new SimpleAction ("edit-entry", VariantType.STRING);
		action.activate.connect ((_action, param) => {
			Gtk.Entry entry = null;
			string title = null;
			switch (param.get_string ()) {
			case "value":
				entry = value;
				title = _(token.code.description);
				break;
			case "prompt":
				entry = prompt;
				title = _("Title");
				break;
			case "default-value":
				entry = default_value;
				title = _("Default Value");
				break;
			}
			if (entry != null) {
				edit_entry (title, entry);
			}
		});
		action_group.add_action (action);

		action = new SimpleAction ("hide-value", null);
		action.activate.connect ((_action, param) => {
			edit_value_box.visible = false;
			edit_value_button.visible = false;
			hide_value_button.visible = false;
			if (token.code.type == TemplateCodeType.DATE) {
				date_format.visible = true;
				date_format.selected = 0;
			}
		});
		action_group.add_action (action);
	}

	public string append_value (StringBuilder str) {
		switch (token.code.type) {
		case TemplateCodeType.SPACE:
			str.append_c (' ');
			break;

		case TemplateCodeType.TEXT:
			str.append (value.text);
			break;

		case TemplateCodeType.ENUMERATOR:
			//var digits = enumerator_digits.get_value_as_int ();
			//for (var i = 0; i < digits; i++) {
			//	str.append_c ('#');
			//}
			break;

		case TemplateCodeType.SIMPLE:
			str.append_c ('%');
			str.append_unichar (token.code.code);
			break;

		case TemplateCodeType.DATE:
			str.append_c ('%');
			str.append_unichar (token.code.code);
			if (date_format.selected == DATE_FORMATS.length) {
				if (!Strings.empty (value.text)) {
					str.append_printf ("{ %s }", value.text);
				}
				else {
					str.append_printf ("{ %s }", DEFAULT_STRFTIME_FORMAT);
				}
			}
			else {
				str.append_printf ("{ %s }", DATE_FORMATS[date_format.selected]);
			}
			break;

		case TemplateCodeType.FILE_ATTRIBUTE:
			var attribute = attributes.get ((int) attribute_selector.selected);
			if (attribute != null) {
				str.append_c ('%');
				str.append_unichar (token.code.code);
				str.append_printf ("{ %s }", attribute.info.id);
			}
			break;

		case TemplateCodeType.QUOTED:
			str.append_c ('%');
			str.append_unichar (token.code.code);
			str.append_printf ("{ %s }", value.text);
			break;

		case TemplateCodeType.ASK_VALUE:
			str.append_c ('%');
			str.append_unichar (token.code.code);
			str.append_printf ("{ %s }", prompt.text);
			str.append_printf ("{ %s }", default_value.text);
			break;
		}
		return "";
	}

	construct {
		value.changed.connect (() => data_changed ());
		prompt.changed.connect (() => data_changed ());
		default_value.changed.connect (() => data_changed ());
		date_format.notify["selected"].connect (() => data_changed ());
		attribute_selector.notify["selected"].connect (() => data_changed ());
	}

	[GtkChild] unowned Gtk.Label title;
	[GtkChild] unowned Gtk.Entry value;
	[GtkChild] unowned Gtk.Box extra_content;
	[GtkChild] unowned Gtk.Box edit_value_box;
	[GtkChild] unowned Gtk.Button edit_value_button;
	[GtkChild] unowned Gtk.Entry prompt;
	[GtkChild] unowned Gtk.Entry default_value;
	[GtkChild] unowned Gtk.DropDown date_format;
	[GtkChild] unowned Gtk.Button hide_value_button;
	[GtkChild] unowned Gtk.DropDown attribute_selector;
	GenericList<AttributeItem> attributes;
	int hot_x;
	int hot_y;

	const string[] DATE_FORMATS = {
		"%Y-%m-%d--%H.%M.%S",
		"%x %X",
		"%Y%m%d%H%M%S",
		"%Y-%m-%d",
		"%x",
		"%Y%m%d",
		"%H.%M.%S",
		"%X",
		"%H%M%S"
	};
}


class Gth.AttributeItem : Object {
	public unowned MetadataInfo info;
	public unowned MetadataCategory category;

	public string title {
		get {
			if (!Strings.empty (info.display_name)) {
				return _(info.display_name);
			}
			else {
				return info.id;
			}
		}
	}
	public AttributeItem (MetadataInfo _info) {
		info = _info;
		category = MetadataCategory.get (info.category);
	}
}
