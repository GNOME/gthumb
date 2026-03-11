[GtkTemplate (ui = "/app/gthumb/gthumb/ui/print-layout-dialog.ui")]
public class Gth.PrintLayoutDialog : Adw.Window {
	public PrintDialogResult result;

	public PrintLayoutDialog (Gth.MainWindow _window, Gth.PrintLayout _print_layout) {
		window = _window;
		result = PrintDialogResult.CANCEL;

		print_layout = _print_layout;
		print_layout.notify["total-pages"].connect (() => update_total_pages ());
		print_layout.notify["current-page"].connect (() => update_current_page ());
		print_layout.changed.connect (() => update_selected_image_layout ());
		print_layout.image_changed.connect (() => update_selected_image_layout ());

		scale_adjustment.value_changed.connect ((local_adj) => {
			if ((preview.selected_image != null) && (initializing == 0)) {
				preview.selected_image.zoom = (float) (local_adj.value / 100);
				update_selected_image_layout ();
			}
		});

		initializing++;
		preview.set_print_layout (print_layout);
		preview.notify["selected-image"].connect (() => update_image_options ());
		update_page_options ();
		update_total_pages ();
		update_current_page ();
		layout_rows.value = print_layout.rows;
		layout_columns.value = print_layout.columns;
		initializing--;
	}

	void update_selected_image_layout () {
		if (preview.selected_image == null) {
			return;
		}
		print_layout.update_image_layout (preview.selected_image);
		update_image_position_controls ();
		preview.queue_draw ();
	}

	void update_total_pages () {
		total_pages.label = " / %d".printf (print_layout.total_pages);
	}

	void update_current_page () {
		current_page.label = "%d".printf (print_layout.current_page);
	}

	[GtkCallback]
	void on_next_page (Gtk.Button button) {
		print_layout.next_page ();
	}

	[GtkCallback]
	void on_previous_page (Gtk.Button button) {
		print_layout.previous_page ();
	}

	[GtkCallback]
	void on_cancel (Gtk.Button button) {
		result = PrintDialogResult.CANCEL;
		close ();
	}

	[GtkCallback]
	void on_setup (Gtk.Button button) {
		result = PrintDialogResult.SETUP;
		close ();
	}

	[GtkCallback]
	void on_print (Gtk.Button button) {
		result = PrintDialogResult.PRINT;
		close ();
	}

	[GtkCallback]
	void on_layout_changed (Gtk.Adjustment adj) {
		if (initializing > 0) {
			return;
		}
		print_layout.rows = (int) layout_rows.value;
		print_layout.columns = (int) layout_columns.value;
		print_layout.changed_layout (Recenter.RECENTER);
	}

	[GtkCallback]
	void on_image_orientation_changed (Object obj, ParamSpec param) {
		var toggle_group = obj as Adw.ToggleGroup;
		if (preview.selected_image != null) {
			var new_orientation = Orientation.from_state (toggle_group.active_name);
			if (new_orientation != preview.selected_image.orientation) {
				preview.selected_image.orientation = new_orientation;
				update_selected_image_layout ();
			}
		}
	}

	void update_image_options () {
		if (preview.selected_image == null) {
			image_group.sensitive = false;
			return;
		}
		image_group.sensitive = true;
		image_orientation_group.active_name = preview.selected_image.orientation.to_state ();
		scale_adjustment.value = preview.selected_image.zoom * 100;
		update_image_position_controls ();
	}

	void update_page_options () {
		if (print_layout.get_page_is_rotated ()) {
			page_orientation_group.active_name = "landscape";
		}
		else {
			page_orientation_group.active_name = "portrait";
		}
		update_length_adjustments ();
	}

	[GtkCallback]
	void on_unit_changed (Object obj, ParamSpec param) {
		if (initializing > 0) {
			return;
		}
		var toggle_group = obj as Adw.ToggleGroup;
		print_layout.default_unit = PrintUnit.from_state (toggle_group.active_name);
		initializing++;
		update_length_adjustments ();
		initializing--;
		print_layout.changed_layout ();
		preview.queue_resize ();
	}

	void update_length_adjustments () {
		var unit_state = print_layout.default_unit.to_state ();
		margin_unit_group.active_name = unit_state;
		position_unit_group.active_name = unit_state;

		var gtk_unit = print_layout.default_unit.to_gtk_unit ();
		update_adjustments_for_unit (page_horizontal_margin, print_layout.page_setup.get_left_margin (gtk_unit));
		update_adjustments_for_unit (page_vertical_margin, print_layout.page_setup.get_top_margin (gtk_unit));
	}

	void update_adjustments_for_unit (Adw.SpinRow spin_row, double new_value) {
		if (print_layout.default_unit == PrintUnit.MM) {
			spin_row.adjustment.configure (new_value, 0, 50, 1, 5, 0);
			spin_row.digits = 0;
		}
		else if (print_layout.default_unit == PrintUnit.INCH) {
			spin_row.adjustment.configure (new_value, 0, 5, 0.05, 0.1, 0);
			spin_row.digits = 2;
		}
	}

	[GtkCallback]
	void on_page_orientation_changed (Object obj, ParamSpec param) {
		if (initializing > 0) {
			return;
		}
		initializing++;
		var toggle_group = obj as Adw.ToggleGroup;
		if (toggle_group.active_name == "landscape") {
			var tmp = page_horizontal_margin.value;
			page_horizontal_margin.value = page_vertical_margin.value;
			page_vertical_margin.value = tmp;
			print_layout.set_page_orientation (Gtk.PageOrientation.LANDSCAPE);
		}
		else {
			var tmp = page_horizontal_margin.value;
			page_horizontal_margin.value = page_vertical_margin.value;
			page_vertical_margin.value = tmp;
			print_layout.set_page_orientation (Gtk.PageOrientation.PORTRAIT);
		}
		initializing--;
		print_layout.changed_layout ();
		preview.queue_resize ();
	}

	[GtkCallback]
	void on_page_horizontal_margin_changed (Gtk.Adjustment adj) {
		if (initializing > 0) {
			return;
		}
		if (!print_layout.get_page_is_rotated ()) {
			print_layout.set_horizontal_margin (adj.value);
		}
		else {
			print_layout.set_vertical_margin (adj.value);
		}
		print_layout.changed_layout ();
	}

	[GtkCallback]
	void on_page_vertical_margin_changed (Gtk.Adjustment adj) {
		if (initializing > 0) {
			return;
		}
		if (!print_layout.get_page_is_rotated ()) {
			print_layout.set_vertical_margin (adj.value);
		}
		else {
			print_layout.set_horizontal_margin (adj.value);
		}
		print_layout.changed_layout ();
	}

	void update_image_position_controls () {
		if (preview.selected_image == null) {
			return;
		}

		double step_length, page_length;
		int digits;
		if (print_layout.default_unit == PrintUnit.MM) {
			step_length = 1;
			page_length = 10;
			digits = 0;
		}
		else {
			step_length = 0.05;
			page_length = 0.1;
			digits = 2;
		}

		initializing++;
		var image = preview.selected_image;
		image_x.adjustment.configure (
			image.image_box.origin.x,
			image.bounding_box.origin.x,
			image.bounding_box.origin.x + (image.bounding_box.size.width - image.image_box.size.width),
			step_length, page_length,
			0
		);
		image_y.adjustment.configure (
			image.image_box.origin.y,
			image.bounding_box.origin.y,
			image.bounding_box.origin.y + (image.bounding_box.size.height - image.image_box.size.height),
			step_length, page_length,
			0
		);
		image_width.adjustment.configure (
			image.image_box.size.width,
			0,
			image.maximized_box.size.width,
			step_length, page_length,
			0
		);
		image_height.adjustment.configure (
			image.image_box.size.height,
			0,
			image.maximized_box.size.height,
			step_length, page_length,
			0
		);
		image_x.digits = digits;
		image_y.digits = digits;
		image_width.digits = digits;
		image_height.digits = digits;
		initializing--;
		scale_adjustment.value = image.zoom * 100;
	}

	[GtkCallback]
	void on_image_x_changed (Object obj, ParamSpec param) {
		if ((initializing > 0) || (preview.selected_image == null)) {
			return;
		}
		var spin_row = obj as Adw.SpinRow;
		preview.selected_image.transform.x = (float) ((spin_row.value - spin_row.adjustment.lower) / (spin_row.adjustment.upper - spin_row.adjustment.lower));
		print_layout.update_image_layout (preview.selected_image);
		preview.queue_draw ();
	}

	[GtkCallback]
	void on_image_y_changed (Object obj, ParamSpec param) {
		if ((initializing > 0) || (preview.selected_image == null)) {
			return;
		}
		var spin_row = obj as Adw.SpinRow;
		preview.selected_image.transform.y = (float) ((spin_row.value - spin_row.adjustment.lower) / (spin_row.adjustment.upper - spin_row.adjustment.lower));
		print_layout.update_image_layout (preview.selected_image);
		preview.queue_draw ();
	}

	[GtkCallback]
	void on_image_size_changed (Object obj, ParamSpec param) {
		if ((initializing > 0) || (preview.selected_image == null)) {
			return;
		}
		var spin_row = obj as Adw.SpinRow;
		preview.selected_image.zoom = (float) (spin_row.value / spin_row.adjustment.upper);
		update_selected_image_layout ();
	}

	[GtkCallback]
	void on_center_image_horizontal (Gtk.Button button) {
		if (preview.selected_image == null) {
			return;
		}
		preview.selected_image.recenter_at_next_update (Recenter.RECENTER_X);
		update_selected_image_layout ();
	}

	[GtkCallback]
	void on_center_image_vertical (Gtk.Button button) {
		if (preview.selected_image == null) {
			return;
		}
		preview.selected_image.recenter_at_next_update (Recenter.RECENTER_Y);
		update_selected_image_layout ();
	}

	[GtkChild] unowned Gth.PrintPreview preview;
	[GtkChild] unowned Adw.SpinRow layout_rows;
	[GtkChild] unowned Adw.SpinRow layout_columns;
	[GtkChild] unowned Gtk.Label current_page;
	[GtkChild] unowned Gtk.Label total_pages;
	[GtkChild] unowned Adw.PreferencesGroup image_group;
	[GtkChild] unowned Adw.ToggleGroup image_orientation_group;
	[GtkChild] unowned Gtk.Adjustment scale_adjustment;
	[GtkChild] unowned Adw.ToggleGroup page_orientation_group;
	[GtkChild] unowned Adw.ToggleGroup margin_unit_group;
	[GtkChild] unowned Adw.ToggleGroup position_unit_group;
	[GtkChild] unowned Adw.SpinRow page_horizontal_margin;
	[GtkChild] unowned Adw.SpinRow page_vertical_margin;
	[GtkChild] unowned Adw.SpinRow image_x;
	[GtkChild] unowned Adw.SpinRow image_y;
	[GtkChild] unowned Adw.SpinRow image_width;
	[GtkChild] unowned Adw.SpinRow image_height;

	Gth.MainWindow window;
	Gth.PrintLayout print_layout;
	int initializing = 0;
}

public enum Gth.PrintDialogResult {
	PRINT,
	SETUP,
	CANCEL,
}
