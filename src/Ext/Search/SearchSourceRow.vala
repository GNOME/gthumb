[GtkTemplate (ui = "/org/gnome/gthumb/ui/search-source-row.ui")]
public class Gth.SearchSourceRow : Adw.ActionRow {
	public Gth.SearchSource search_source;

	public signal void move_to_row (Gth.SearchSourceRow row);
	public signal void delete_row ();

	public SearchSourceRow (SearchSource _search_source, bool as_icon_content = false) {
		search_source = _search_source;
		set_folder (search_source.folder);

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
			icon_widget.append (new Gth.SearchSourceRow (search_source, true));

			unowned var drag_icon = Util.get_drag_icon_for_drag (drag);
			drag_icon.set_child (icon_widget);
			drag.set_hotspot (hot_x, hot_y);
		});
		add_controller (drag_source);

		var drop_target = new Gtk.DropTarget (typeof (Gth.SearchSourceRow), Gdk.DragAction.MOVE);
		drop_target.drop.connect ((target, value, x, y) => {
			var source = value.get_object () as Gth.SearchSourceRow;
			if ((source == null) || (source == this) || (source.parent != this.parent)) {
				return false;
			}
			source.move_to_row (this);
			return true;
		});
		add_controller (drop_target);

		var action_group = new SimpleActionGroup ();
		insert_action_group ("row", action_group);

		var action = new SimpleAction ("delete", null);
		action.activate.connect ((_action, param) => {
			delete_row ();
		});
		action_group.add_action (action);

		action = new SimpleAction.stateful ("recursive", null, new Variant.boolean (search_source.recursive));
		action.activate.connect ((_action, param) => {
			search_source.recursive = Util.toggle_state (_action);
		});
		action_group.add_action (action);
	}

	[GtkCallback]
	void on_activated (Adw.ActionRow row) {
		var selector = new Gth.FolderSelector ();
		selector.select_folder.begin (get_root () as Gth.MainWindow, search_source.folder, null, (obj, _res) => {
			try {
				var folder = selector.select_folder.end (_res);
				set_folder (folder);
			}
			catch (Error error) {
			}
		});
	}

	void set_folder (File folder) {
		if (folder != search_source.folder) {
			search_source.folder = folder;
		}
		var file_source = app.get_source_for_file (search_source.folder);
		if (file_source != null) {
			var info = file_source.get_display_info (search_source.folder);
			title = info.get_display_name ();
			icon.set_from_gicon (info.get_symbolic_icon ());
		}
	}

	[GtkChild] unowned Gtk.Image icon;

	int hot_x;
	int hot_y;
}
