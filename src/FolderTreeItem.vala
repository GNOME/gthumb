public class Gth.FolderTreeItem : Gtk.Box {
	const int ROW_H_PADDING = 12;
	const int ICON_SPACING = 6;

	public FolderTreeItem (Gth.FolderTree _folder_tree) {
		folder_tree = _folder_tree;
		row_expended_id = 0;
		file_renamed_id = 0;
		children_state_changed_id = 0;

		expander = new Gtk.TreeExpander ();
		expander.margin_start = ROW_H_PADDING;
		expander.margin_end = ROW_H_PADDING;
		expander.hide_expander = false;
		expander.indent_for_icon = true;
		append (expander);

		var box = new Gtk.Box (Gtk.Orientation.HORIZONTAL, ICON_SPACING);
		expander.set_child (box);

		icon = new Gtk.Image ();
		icon.icon_size = Gtk.IconSize.NORMAL;
		box.append (icon);

		label = new Gtk.Label ("");
		label.ellipsize = Pango.EllipsizeMode.MIDDLE;
		label.halign = Gtk.Align.START;
		box.append (label);

		var controller = new Gtk.GestureClick ();
		controller.released.connect ((n_press, x, y) => {
			var row = expander.get_list_row ();
			if (row != null) {
				if (FileData.equal (file_data, folder_tree.current_folder)) {
					row.expanded = !row.expanded;
				}
			}
		});
		add_controller (controller);

		var secondary_button = new Gtk.GestureClick ();
		secondary_button.set_button (Gdk.BUTTON_SECONDARY);
		secondary_button.pressed.connect ((n_press, x, y) => {
			folder_tree.open_context_menu (this, (int) x, (int) y);
		});
		add_controller (secondary_button);
	}

	public void bind (Gtk.TreeListRow row, FileData _file_data) {
		file_data = _file_data;
		label.set_text (file_data.info.get_display_name ());
		icon.set_from_gicon (file_data.get_symbolic_icon ());
		expander.list_row = row;
		update_expander_visibility ();
		row_expended_id = row.notify["expanded"].connect ((obj, _spec) => {
			if (expander.list_row.expanded) {
				folder_tree.list_subfolders (file_data);
			}
		});
		file_renamed_id = file_data.info_changed.connect_after (() => {
			label.set_text (file_data.info.get_display_name ());
		});
		children_state_changed_id = file_data.notify["children-state"].connect (() => {
			update_expander_visibility ();
		});
	}

	public void unbind () {
		expander.list_row.disconnect (row_expended_id);
		file_data.disconnect (file_renamed_id);
		file_data.disconnect (children_state_changed_id);
		expander.list_row = null;
		file_data = null;
	}

	void update_expander_visibility () {
		expander.hide_expander = file_data.info.get_attribute_boolean ("gthumb::no-child")
			|| ((file_data.children_state == ChildrenState.LOADED) && file_data.children.is_empty ());
	}

	weak Gth.FolderTree folder_tree;
	public FileData file_data;
	Gtk.TreeExpander expander;
	Gtk.Image icon;
	Gtk.Label label;
	ulong row_expended_id;
	ulong file_renamed_id;
	ulong children_state_changed_id;
}
