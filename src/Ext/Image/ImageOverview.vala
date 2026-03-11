[GtkTemplate (ui = "/app/gthumb/gthumb/ui/image-overview.ui")]
public class Gth.ImageOverview : Gtk.Box {
	public Gth.ImageView main_view {
		set {
			_main_view = value;
			_main_view.resized.connect (() => update_selection ());
			_main_view.notify["image"].connect (() => preview.image = _main_view.image);
			hadj_changed = _main_view.hadjustment.value_changed.connect ((adj) => update_selection ());
			vadj_changed = _main_view.vadjustment.value_changed.connect ((adj) => update_selection ());
			preview.image = _main_view.image;
			Util.enable_action (action_group, "set-zoom", _main_view != null);
			Util.enable_action (action_group, "view-all", _main_view != null);
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
		preview.set_selection (viewport);

		SignalHandler.block (zoom_scale.adjustment, zoom_changed_id);
		zoom_scale.adjustment.lower = ZoomScale.get_adj_value (_main_view.min_zoom, _main_view.min_zoom, _main_view.max_zoom);
		zoom_scale.adjustment.upper = ZoomScale.get_adj_value (_main_view.max_zoom, _main_view.min_zoom, _main_view.max_zoom);
		zoom_scale.adjustment.value = ZoomScale.get_adj_value (_main_view.zoom, _main_view.min_zoom, _main_view.max_zoom);
		SignalHandler.unblock (zoom_scale.adjustment, zoom_changed_id);
	}

	construct {
		_main_view = null;
		hadj_changed = 0;
		vadj_changed = 0;

		zoom_scale.adjustment.value = ZoomScale.get_adj_value (1.0f);
		zoom_changed_id = zoom_scale.adjustment.value_changed.connect ((adj) => {
			if (_main_view != null) {
				var new_zoom = ZoomScale.get_zoom (adj.value, _main_view.min_zoom, _main_view.max_zoom);
				_main_view.zoom = new_zoom;
			}
		});

		preview.boundless_zoom = true;
		preview.resized.connect (() => update_selection ());

		var drag_events = new Gtk.GestureDrag ();
		drag_events.drag_begin.connect ((x, y) => {
			preview.cursor = new Gdk.Cursor.from_name ("grabbing", null);
			move_to (x, y);
		});
		drag_events.drag_end.connect ((x, y) => {
			preview.cursor = null;
		});
		drag_events.drag_update.connect ((local_drag_events, ofs_x, ofs_y) => {
			double start_x, start_y;
			local_drag_events.get_start_point (out start_x, out start_y);
			move_to (start_x + ofs_x, start_y + ofs_y);
		});
		preview.add_controller (drag_events);

		var scroll_events = new Gtk.EventControllerScroll (Gtk.EventControllerScrollFlags.VERTICAL);
		scroll_events.scroll.connect ((controller, dx, dy) => {
			if (_main_view != null) {
				var step = (dy < 0) ? 0.1f : -0.1f;
				var new_zoom = _main_view.zoom + (_main_view.zoom * step);
				_main_view.zoom = new_zoom;
			}
		});
		add_controller (scroll_events);

		action_group = new SimpleActionGroup ();
		insert_action_group ("overview", action_group);

		var action = new SimpleAction ("view-all", null);
		action.activate.connect (() => _main_view.zoom_type = ZoomType.MAXIMIZE_IF_LARGER);
		action_group.add_action (action);

		action = new SimpleAction ("set-zoom", VariantType.DOUBLE);
		action.activate.connect ((obj, param) => _main_view.zoom = (float) param.get_double ());
		action_group.add_action (action);
	}

	void move_to (double x, double y) {
		var z = _main_view.zoom / preview.zoom;
		x = (x - preview.texture_box.origin.x - (viewport.size.width / 2)) * z;
		y = (y - preview.texture_box.origin.y - (viewport.size.height / 2)) * z;
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
	SimpleActionGroup action_group;
}
