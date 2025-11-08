public class Gth.FilterGrid : Gtk.Box {
	public signal void activated (uint id);

	public void add (uint id, ImageOperation operation, string title, string? tooltip = null) {
		var preview = new Gtk.Picture ();
		var label = new Gtk.Label (title);
		var button_content = new Gtk.Box (Gtk.Orientation.VERTICAL, 6);
		button_content.append (preview);
		button_content.append (label);

		var button = new Gtk.ToggleButton ();
		button.child = button_content;
		button.add_css_class ("filter-preview");
		if (tooltip != null) {
			button.tooltip_text = tooltip;
		}
		button.action_name = "grid.set-filter";
		button.action_target = new Variant.uint32 (id);
		grid.append (button);

		var cell = new Cell () {
			button = button,
			preview = preview,
			label = label,
			operation = operation
		};
		cells.set (id, cell);
	}

	public void update_previews (Gth.Image image, Cancellable cancellable) {
		var preview_size = uint.max (image.width, image.height);
		foreach (var id in cells.get_keys ()) {
			var cell = cells.get (id);
			//var filter_preview = app.image_editor.exec_operation (image, cell.operation, cancellable);
			var filter_preview = cell.operation.execute (image, cancellable);
			if (filter_preview != null) {
				cell.preview.paintable = filter_preview.get_texture ();
				cell.preview.set_size_request ((int) preview_size, (int) preview_size);
			}
		}
	}

	public void activate (uint id) {
		Util.set_state (action_group, "set-filter", new Variant.uint32 (id));
		activated (id);
	}

	public uint get_activated () {
		var action = action_group.lookup_action ("set-filter") as SimpleAction;
		if (action == null) {
			return 0;
		}
		var state = action.get_state ();
		return state.get_uint32 ();
	}

	construct {
		orientation = Gtk.Orientation.VERTICAL;
		grid = new Gtk.FlowBox ();
		grid.row_spacing = 20;
		grid.column_spacing = 20;
		grid.max_children_per_line = 3;
		grid.halign = Gtk.Align.CENTER;
		grid.selection_mode = Gtk.SelectionMode.NONE;
		append (grid);
		cells = new HashTable<uint, Cell?> (null, null);

		action_group = new SimpleActionGroup ();
		insert_action_group ("grid", action_group);

		var action = new SimpleAction.stateful ("set-filter", VariantType.UINT32, new Variant.uint32 (0));
		action.activate.connect ((action, param) => {
			var old_id = action.get_state ().get_uint32 ();
			var new_id = param.get_uint32 ();
			if (old_id == new_id) {
				new_id = 0;
			}
			action.set_state (new Variant.uint32 (new_id));
			activated (new_id);
		});
		action_group.add_action (action);
	}

	struct Cell {
		unowned Gtk.ToggleButton button;
		unowned Gtk.Picture preview;
		unowned Gtk.Label label;
		Gth.ImageOperation operation;
	}

	Gtk.FlowBox grid;
	HashTable <uint, Cell?> cells;
	SimpleActionGroup action_group;

	const int PREVIEW_SIZE = 110;
}
