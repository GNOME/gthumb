public class Gth.ImageAlignment : Object, ImageController {
	public signal void changed ();

	public override void set_view (ImageView? view) {
		if (_view != null) {
			_view.remove_controller (drag_events);
		}
		_view = view;
		if (_view == null) {
			return;
		}
		_view.add_controller (drag_events);
	}

	public override void on_snapshot (Gtk.Snapshot snapshot) {
		snapshot.push_blend (Gsk.BlendMode.DIFFERENCE);

		_view.snapshot_image (snapshot);

		snapshot.pop ();

		var builder = new Gsk.PathBuilder ();
		builder.move_to (drag_start.x, drag_start.y);
		builder.line_to (drag_end.x, drag_end.y);
		snapshot.append_stroke (builder.to_path (), new Gsk.Stroke (2), { 1, 1, 1, 1 });

		snapshot.pop ();
	}

	public float get_radiants (Gtk.Orientation orientation) {
		double angle;
		if (orientation == Gtk.Orientation.HORIZONTAL) {
			if (drag_start.y == drag_end.y) {
				return 0;
			}
			if (drag_end.x > drag_start.x) {
				angle = - Math.atan2 (drag_end.y - drag_start.y, drag_end.x - drag_start.x);
			}
			else {
				angle = - Math.atan2 (drag_start.y - drag_end.y, drag_start.x - drag_end.x);
			}
		}
		else {
			if (drag_start.x == drag_end.x) {
				return 0;
			}
			if (drag_end.y > drag_start.y) {
				angle = Math.atan2 (drag_end.x - drag_start.x, drag_end.y - drag_start.y);
			}
			else {
				angle = Math.atan2 (drag_start.x - drag_end.x, drag_start.y - drag_end.y);
			}
		}
		return (float) angle;
	}

	construct {
		_view = null;

		drag_events = new Gtk.GestureDrag ();
		drag_events.drag_begin.connect ((x, y) => {
			drag_start = PointUtil.point_from_click (x, y);
		});
		drag_events.drag_end.connect ((x, y) => {
			changed ();
		});
		drag_events.drag_update.connect ((local_drag_events, ofs_x, ofs_y) => {
			double start_x, start_y;
			local_drag_events.get_start_point (out start_x, out start_y);
			drag_end = PointUtil.point_from_click ((float) (start_x + ofs_x), (float) (start_y + ofs_y));
			_view.queue_draw ();
		});
	}

	Gtk.GestureDrag drag_events;
	ImageView _view;
	Graphene.Point drag_start;
	Graphene.Point drag_end;
}
