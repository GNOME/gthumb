public class Gth.PrintPreview : Gtk.Widget {
	public Gth.PrintLayout print_layout;

	public PrintImageLayout? selected_image { get; set; default = null; }

	construct {
		add_css_class ("print-view");
		pointer_cursor = new Gdk.Cursor.from_name ("pointer", null);

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((x, y) => {
			if (dragging) {
				return;
			}
			Graphene.Point point = { (float) x, (float) y };
			image_under_pointer = get_image_at_point (point);
			cursor = (image_under_pointer != null) ? pointer_cursor : null;
			queue_draw ();
		});
		motion_events.leave.connect (() => {
			if (dragging) {
				return;
			}
			image_under_pointer = null;
			cursor = null;
			queue_draw ();
		});
		add_controller (motion_events);

		var click_events = new Gtk.GestureClick ();
		click_events.button = Gdk.BUTTON_PRIMARY;
		click_events.pressed.connect ((n_press, x, y) => {
			if (image_under_pointer != null) {
				selected_image = image_under_pointer;
			}
			else {
				selected_image = null;
			}
			queue_draw ();
		});
		add_controller (click_events);

		var drag_events = new Gtk.GestureDrag ();
		drag_events.drag_begin.connect ((x, y) => {
			if (selected_image == null) {
				return;
			}
			var bounding_box = get_scaled_box (selected_image.image_box);
			Graphene.Point point = { (float) x, (float) y };
			if (!bounding_box.contains_point (point)) {
				return;
			}
			prev_cursor = cursor;
			cursor = new Gdk.Cursor.from_name ("grabbing", null);
			drag_start = PointUtil.point_from_click (x, y);
			dragging = true;
		});
		drag_events.drag_end.connect ((x, y) => {
			cursor = prev_cursor;
			dragging = false;
		});
		drag_events.drag_update.connect ((local_drag_events, ofs_x, ofs_y) => {
			if (!dragging) {
				return;
			}
			double start_x, start_y;
			local_drag_events.get_start_point (out start_x, out start_y);
			double x = start_x + ofs_x;
			double y = start_y + ofs_y;
			move_selected_image_by (x - drag_start.x, y - drag_start.y);
			drag_start = PointUtil.point_from_click (x, y);
		});
		add_controller (drag_events);
	}

	void move_selected_image_by (double dx, double dy) {
		if (selected_image == null) {
			return;
		}
		// selected_image.transform.x += (float) (dx / _zoom);
		// selected_image.transform.y += (float) (dy / _zoom);
		// print_layout.update_image_layout (selected_image);
		print_layout.move_image_by (selected_image, (float) (dx / _zoom), (float) (dy / _zoom));
		queue_draw ();
	}

	bool dragging = false;
	Graphene.Point drag_start;
	Gdk.Cursor pointer_cursor;
	Gdk.Cursor prev_cursor;
	PrintImageLayout image_under_pointer = null;

	Graphene.Rect get_scaled_box (Graphene.Rect image_box, int padding = 0) {
		Graphene.Rect scaled_box = {
			{
				paper_box.origin.x + (image_box.origin.x * _zoom) - padding,
				paper_box.origin.y + (image_box.origin.y * _zoom) - padding,
			},
			{
				(image_box.size.width * _zoom) + (padding * 2),
				(image_box.size.height * _zoom) + (padding * 2),
			}
		};
		return scaled_box;
	}

	PrintImageLayout? get_image_at_point (Graphene.Point point) {
		foreach (unowned var image in print_layout.images) {
			if (image.page != print_layout.current_page) {
				continue;
			}
			var bounding_box = get_scaled_box (image.bounding_box);
			if (bounding_box.contains_point (point)) {
				return image;
			}
		}
		return null;
	}

	public void set_print_layout (Gth.PrintLayout _print_layout) {
		print_layout = _print_layout;
		print_layout.changed.connect (() => {
			update_selected_image_from_layout ();
			queue_draw ();
		});
	}

	void update_selected_image_from_layout () {
		if ((selected_image != null) && (selected_image.page == print_layout.current_page)) {
			return;
		}

		if (print_layout.images.length == 1) {
			selected_image = print_layout.images[0];
			return;
		}

		if ((print_layout.rows == 1) && (print_layout.columns == 1)) {
			foreach (unowned var image in print_layout.images) {
				if (image.page == print_layout.current_page) {
					selected_image = image;
					return;
				}
			}
		}

		selected_image = null;
	}

	public override Gtk.SizeRequestMode get_request_mode () {
		return Gtk.SizeRequestMode.HEIGHT_FOR_WIDTH;
	}

	public override void measure (Gtk.Orientation orientation, int for_size, out int minimum, out int natural, out int minimum_baseline, out int natural_baseline) {
		minimum = 0;
		natural = 0;
		natural_baseline = -1;
		minimum_baseline = -1;
	}

	public override void size_allocate (int width, int height, int baseline) {
		var natural_width = (uint) ((print_layout != null) ? print_layout.page_setup.get_paper_width (Gtk.Unit.MM) : MINIMUM_SIZE);
		var natural_height = (uint) ((print_layout != null) ? print_layout.page_setup.get_paper_height (Gtk.Unit.MM) : MINIMUM_SIZE);
		_zoom = Util.get_zoom_to_fit_surface (natural_width, natural_height, width - PAPER_MARGIN, height - PAPER_MARGIN);
		var zoomed_width = _zoom * natural_width;
		var zoomed_height = _zoom * natural_height;
		// stdout.printf ("> natural: %u,%u\n", natural_width, natural_height);
		// stdout.printf ("  max: %d,%d\n", width, height);
		// stdout.printf ("  zoomed: %f,%f\n", zoomed_width, zoomed_height);
		// stdout.printf ("  zoom: %f\n", _zoom);
		paper_box = { {
				Util.center_content (width, zoomed_width),
				Util.center_content (height, zoomed_height)
			}, {
				zoomed_width,
				zoomed_height
			}
		};
		if (print_layout != null) {
			print_layout.update ();
		}
	}

	public override void snapshot (Gtk.Snapshot snapshot) {
		snapshot.save ();

		Gdk.RGBA white = { 1, 1, 1, 1 };
		snapshot.append_color (white, paper_box);

		if (print_layout != null) {
			Gdk.RGBA gray = { 0.75f, 0.75f, 0.75f, 1 };
			Gdk.RGBA frame_color = { 0.9f, 0.9f, 0.9f, 1 };
			Gdk.RGBA selected_color = { 0.85f, 0.15f, 0.30f, 1 };
			Gdk.RGBA preactive_color = { 0.5f, 0.5f, 0.5f, 1 };
			Gdk.RGBA preactive_selected_color = { 0.85f, 0.15f, 0.30f, 1 };

			foreach (unowned var image in print_layout.images) {
				if (image.page != print_layout.current_page) {
					continue;
				}

				// Image

				Graphene.Rect image_box = get_scaled_box (image.image_box);
				image.snapshot (snapshot, paper_box.size, image_box);

				// Frame

				Graphene.Rect bounding_box = get_scaled_box (image.bounding_box, 1);
				var path = new Gsk.PathBuilder ();
				path.add_rect (bounding_box);
				var preactive = (image == image_under_pointer);
				var selected = (image == selected_image);
				if (selected) {
					var frame_stroke = new Gsk.Stroke (2);
					snapshot.append_stroke (path.to_path (), frame_stroke, preactive ? preactive_selected_color : selected_color);
				}
				else {
					var frame_stroke = new Gsk.Stroke (2);
					// if (!preactive) {
					// 	frame_stroke.set_dash ({ 3, 3 });
					// }
					snapshot.append_stroke (path.to_path (), frame_stroke, preactive ? preactive_color : frame_color);
				}
			}
		}

		snapshot.restore ();
	}

	Graphene.Rect paper_box;
	float _zoom;

	const int MINIMUM_SIZE = 100;
	const int PAPER_MARGIN = 50;
}
