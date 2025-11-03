[GtkTemplate (ui = "/app/gthumb/gthumb/ui/image-overview.ui")]
public class Gth.ImageOverview : Gtk.Box {
	public Gth.ImageView main_view {
		set {
			_main_view = value;
			_main_view.resized.connect (() => update_selection ());
			hadj_changed = _main_view.hadjustment.value_changed.connect ((adj) => update_selection ());
			vadj_changed = _main_view.vadjustment.value_changed.connect ((adj) => update_selection ());
			SignalHandler.block (zoom_scale.adjustment, zoom_changed_id);
			zoom_scale.adjustment.value = ZoomScale.get_adj_value (_main_view.zoom);
			SignalHandler.unblock (zoom_scale.adjustment, zoom_changed_id);
			preview.image = _main_view.image;
		}
	}

	void update_selection () {
		var z = _main_view.zoom / preview.zoom;
		viewport = { {
				(float) (_main_view.hadjustment.value / z),
				(float) (_main_view.vadjustment.value / z)
			}, {
				(float) (_main_view.hadjustment.page_size / z),
				(float) (_main_view.vadjustment.page_size / z)
			}
		};
		// stdout.printf ("> (%f,%f)[%f,%f]\n",
		// 	viewport.origin.x, viewport.origin.y,
		// 	viewport.size.width, viewport.size.height);
		preview.set_selection (viewport);
	}

	construct {
		_main_view = null;
		hadj_changed = 0;
		vadj_changed = 0;
		dragging = false;

		zoom_scale.adjustment.value = ZoomScale.get_adj_value (1.0f);
		zoom_changed_id = zoom_scale.adjustment.value_changed.connect ((adj) => {
			if (_main_view != null) {
				var new_zoom = ZoomScale.get_zoom (adj.value);
				_main_view.zoom = new_zoom;
			}
		});

		var click_events = new Gtk.GestureClick ();
		click_events.pressed.connect ((n_press, x, y) => {
			dragging = true;
			preview.cursor = new Gdk.Cursor.from_name ("grabbing", null);
			move_to (x, y);
		});
		click_events.released.connect ((n_press, x, y) => {
			dragging = false;
			preview.cursor = null;
		});
		add_controller (click_events);

		var motion_events = new Gtk.EventControllerMotion ();
		motion_events.motion.connect ((x, y) => {
			if (dragging) {
				move_to (x, y);
			}
		});
		add_controller (motion_events);

		var scroll_events = new Gtk.EventControllerScroll (Gtk.EventControllerScrollFlags.VERTICAL);
		scroll_events.scroll.connect ((controller, dx, dy) => {
			if (_main_view != null) {
				var step = (dy < 0) ? 0.1f : -0.1f;
				var new_zoom = _main_view.zoom + (_main_view.zoom * step);
				_main_view.zoom = new_zoom;
			}
		});
		add_controller (scroll_events);
	}

	void move_to (double x, double y) {
		var z = _main_view.zoom / preview.zoom;
		x = (x - (viewport.size.width / 2)) * z;
		y = (y - (viewport.size.height / 2)) * z;
		//stdout.printf ("> DRAG: %f,%f\n", x, y);
		_main_view.scroll_to (x, y);
	}

	[GtkChild] unowned Gth.ImageView preview;
	[GtkChild] unowned Gtk.Scale zoom_scale;
	Gth.ImageView _main_view;
	Graphene.Rect viewport;
	ulong hadj_changed;
	ulong vadj_changed;
	ulong zoom_changed_id;
	bool dragging;
}
