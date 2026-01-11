public class Gth.FilterGrid : Gtk.Box {
	public signal void activated (uint id);

	public void add (uint id, ImageOperation operation, string title, string? tooltip = null) {
		var preview = new Gtk.Picture ();
		preview.content_fit = Gtk.ContentFit.SCALE_DOWN;

		var button = new Gtk.ToggleButton ();
		button.child = preview;
		if (tooltip != null) {
			button.tooltip_text = tooltip;
		}
		button.action_name = "grid.set-filter";
		button.action_target = new Variant.uint32 (id);
		button.halign = Gtk.Align.CENTER;

		var label = new Gtk.Label (title);
		label.wrap = true;
		label.justify = Gtk.Justification.CENTER;
		label.halign = Gtk.Align.CENTER;

		var cell_widget = new Gtk.Box (Gtk.Orientation.VERTICAL, 6);
		cell_widget.add_css_class ("filter-preview");
		cell_widget.append (button);
		cell_widget.append (label);
		grid.attach (cell_widget, column, row);

		column++;
		if (column == MAX_COLUMNS) {
			column = 0;
			row++;
		}

		var cell = Cell () {
			operation = operation,
			preview = preview,
		};
		cells.set (id, cell);
	}

	public void update_previews (Gth.Image image, Cancellable cancellable) throws Error {
		var preview_size = uint.max (image.width, image.height);
		foreach (var id in cells.get_keys ()) {
			var cell = cells.get (id);
			var filter_preview = cell.operation.execute (image, cancellable);
			if (filter_preview != null) {
				cell.preview.paintable = filter_preview.get_texture ();
				cell.preview.set_size_request ((int) preview_size, (int) preview_size);
			}
		}
	}

	public void activate_filter (uint id) {
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

	public ImageOperation? get_operation (uint id) {
		var cell = cells.get (id);
		if (cell == null) {
			return null;
		}
		return cell.operation;
	}

	public ImageOperation? get_active_operation () {
		return get_operation (get_activated ());
	}

	construct {
		orientation = Gtk.Orientation.VERTICAL;
		margin_bottom = 24;

		grid = new Gtk.Grid ();
		grid.row_spacing = 20;
		grid.column_spacing = 20;
		grid.halign = Gtk.Align.CENTER;
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

		row = 0;
		column = 0;
	}

	struct Cell {
		Gth.ImageOperation operation;
		unowned Gtk.Picture preview;
	}

	Gtk.Grid grid;
	HashTable <uint, Cell?> cells;
	SimpleActionGroup action_group;
	int row;
	int column;

	const int PREVIEW_SIZE = 110;
	const int MAX_COLUMNS = 2;
}
