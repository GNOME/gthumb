[GtkTemplate (ui = "/org/gnome/gthumb/ui/script-row.ui")]
public class Gth.ScriptRow : Adw.ActionRow {
	public Gth.Script script;

	[GtkChild] public unowned Gtk.Switch visibility_switch;

	public signal void move_to_row (Gth.ScriptRow row);
	public signal void move_to_top ();
	public signal void move_to_bottom ();
	public signal void delete_row ();

	public ScriptRow (Script _script, bool as_icon_content = false) {
		script = _script;
		title = script.display_name;
		activatable = true;
		script.bind_property ("display_name", this, "title", BindingFlags.DEFAULT);
		visibility_switch.active = script.visible;

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
			icon_widget.set_size_request (this.get_width (), this.get_height ());
			icon_widget.append (new Gth.ScriptRow (script, true));

			unowned var drag_icon = Util.get_drag_icon_for_drag (drag);
			drag_icon.set_child (icon_widget);
			drag.set_hotspot (hot_x, hot_y);
		});
		add_controller (drag_source);

		var drop_target = new Gtk.DropTarget (typeof (Gth.ScriptRow), Gdk.DragAction.MOVE);
		drop_target.drop.connect ((target, value, x, y) => {
			var source = value.get_object () as Gth.ScriptRow;
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
	}

	int hot_x;
	int hot_y;
}
