public class Gth.FilterRow : Adw.ActionRow {
	public Gtk.Switch visibility_switch;
	public Gtk.Button edit_button;
	public Gtk.Button delete_button;

	public signal void move_to_row (Gth.FilterRow row);

	public FilterRow (Test filter, bool as_icon_content = false) {
		title = filter.display_name;
		filter.bind_property ("display_name", this, "title", BindingFlags.DEFAULT);

		var icon = new Gtk.Image.from_icon_name ("list-drag-handle-symbolic");
		icon.opacity = 0.5;
		add_prefix (icon);

		if (filter is Gth.Filter) {
			var buttons = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 12);
			buttons.margin_end = 12;
			add_suffix (buttons);

			edit_button = new Gtk.Button.from_icon_name ("edit-item-symbolic");
			edit_button.valign = Gtk.Align.CENTER;
			buttons.append (edit_button);

			delete_button = new Gtk.Button.from_icon_name ("delete-item-symbolic");
			delete_button.add_css_class ("destructive-action");
			delete_button.valign = Gtk.Align.CENTER;
			buttons.append (delete_button);
		}
		else {
			edit_button = null;
			delete_button = null;
		}

		visibility_switch = new Gtk.Switch ();
		visibility_switch.valign = Gtk.Align.CENTER;
		visibility_switch.active = filter.visible;
		add_suffix (visibility_switch);
		activatable_widget = visibility_switch;

		if (as_icon_content)
			return;

		var drag_source = new Gtk.DragSource ();
		drag_source.set_actions (Gdk.DragAction.MOVE);
		drag_source.prepare.connect ((_source, x, y) => {
			hot_x = (int) x;
			hot_y = (int) y;
			return new Gdk.ContentProvider.for_value (this);
		});
		drag_source.drag_begin.connect ((_obj, drag) => {
			var icon_widget = new Gtk.ListBox ();
			//icon_widget.add_css_class ("boxed-list");
			icon_widget.set_size_request (this.get_width (), this.get_height ());
			icon_widget.append (new Gth.FilterRow (filter, true));

			unowned var drag_icon = Gtk.DragIcon.get_for_drag (drag) as Gtk.DragIcon;
			drag_icon.set_child (icon_widget);
			drag.set_hotspot (hot_x, hot_y);
		});
		add_controller (drag_source);

		var drop_target = new Gtk.DropTarget (typeof (Gth.FilterRow), Gdk.DragAction.MOVE);
		drop_target.set_preload (true);
		drop_target.drop.connect ((target, value, x, y) => {
			var source = value.get_object () as Gth.FilterRow;
			if (source == null) {
				return false;
			}
			source.move_to_row (this);
			return true;
		});
		add_controller (drop_target);
	}

	int hot_x;
	int hot_y;
}
