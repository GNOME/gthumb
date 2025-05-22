[GtkTemplate (ui = "/app/gthumb/gthumb/ui/filter-row.ui")]
public class Gth.FilterRow : Adw.ActionRow {
	public Gth.Test filter;

	[GtkChild] public Gtk.Switch visibility_switch;
	[GtkChild] public Gtk.Button edit_button;
	[GtkChild] public Gtk.Button delete_button;

	public signal void move_to_row (Gth.FilterRow row);
	public signal void move_to_top ();
	public signal void move_to_bottom ();

	public FilterRow (Test _filter, bool as_icon_content = false) {
		filter = _filter;
		title = filter.display_name;
		filter.bind_property ("display_name", this, "title", BindingFlags.DEFAULT);
		if (!(filter is Gth.Filter)) {
			edit_button.visible = false;
			delete_button.visible = false;
		}
		visibility_switch.active = filter.visible;

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
	}

	int hot_x;
	int hot_y;
}
