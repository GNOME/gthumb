public class Gth.FolderTreeItem : Gtk.Box {
	const int ROW_H_PADDING = 12;
	const int ICON_SPACING = 6;

	public FolderTreeItem (Gth.FolderTree _folder_tree) {
		folder_tree = _folder_tree;
		job = null;
		row_expended_id = 0;
		file_renamed_id = 0;

		expander = new Gtk.TreeExpander ();
		expander.margin_start = ROW_H_PADDING;
		expander.margin_end = ROW_H_PADDING;
		expander.hide_expander = true;
		expander.indent_for_icon = false;
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
				if (file_data.children_loaded && FileData.equal (file_data, folder_tree.current_folder)) {
					row.expanded = !row.expanded;
				}
			}
		});
		add_controller (controller);
	}

	public void bind (Gtk.TreeListRow row, FileData _file_data) {
		if (job != null) {
			job.cancel ();
			job = null;
		}
		file_data = _file_data;
		label.set_text (file_data.info.get_display_name ());
		icon.set_from_gicon (file_data.get_symbolic_icon ());
		expander.list_row = row;
		row_expended_id = row.notify["expanded"].connect ((obj, _spec) => {
			if (expander.list_row.expanded) {
				job = folder_tree.list_subfolders (file_data);
			}
		});
		file_renamed_id = file_data.renamed.connect (() => {
			label.set_text (file_data.info.get_display_name ());
		});
	}

	public void unbind () {
		expander.list_row.disconnect (row_expended_id);
		file_data.disconnect (file_renamed_id);
		expander.list_row = null;
		if (job != null) {
			job.cancel ();
			job = null;
		}
		file_data = null;
	}

	weak Gth.FolderTree folder_tree;
	public FileData file_data;
	Gtk.TreeExpander expander;
	Gtk.Image icon;
	Gtk.Label label;
	Gth.Job job;
	ulong row_expended_id;
	ulong file_renamed_id;
}
