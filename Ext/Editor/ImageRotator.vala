public class Gth.ImageRotator : Object, ImageController {
	public signal void changed ();
	public signal void changed_position_parameters ();

	// public Graphene.Point center {
	// 	get { return _center; }
	// 	set {
	// 		_center = value;
	// 		// TODO
	// 	}
	// }

	public float radiants {
		set {
			degrees = rad_to_deg (value);
		}
		get {
			return deg_to_rad (_angle);
		}
	}

	public float degrees {
		get { return _angle; }
		set {
			_angle = value;
			if (_view != null) {
				update_clip_area ();
				_view.queue_draw ();
			}
		}
	}

	public RotatedSize rotated_size {
		get { return _rotated_size; }
		set {
			_rotated_size = value;
			update_clip_area ();
			if (_view != null) {
				_view.queue_draw ();
			}
		}
	}

	public Graphene.Rect crop_region {
		get { return _crop_region; }
	}

	public double crop_point1 {
		set {
			move_crop_region (value);
			if (_view != null) {
				_view.queue_draw ();
			}
		}
		get {
			return crop_p1;
		}
	}

	public double crop_point_min {
		get {
			return crop_p_min;
		}
	}

	public double crop_point_max {
		get {
			var max = crop_p1_plus_p2 - crop_p_min;
			return (max - crop_p_min < 0.001) ? crop_p_min : max;
		}
	}

	public Gdk.RGBA background {
		set {
			_background = value;
			if (_view != null) {
				_view.queue_draw ();
			}
		}
		get {
			return _background;
		}
	}

	public void set_view (ImageView? view) {
		if (_view != null) {
			_view.disconnect (view_notify_image_id);
			view_notify_image_id = 0;
			_view.remove_controller (drag_events);
		}

		_view = view;
		if (_view == null) {
			return;
		}

		_view.zoom_limit = ZoomLimit.ALMOST_MAXIMIZE;
		_view.default_zoom_type = ZoomType.MAXIMIZE_IF_LARGER;
		_view.zoom_type = ZoomType.MAXIMIZE_IF_LARGER;
		view_notify_image_id = _view.notify["image"].connect (() => update_image_box ());
		update_image_box ();

		_view.add_controller (drag_events);
		update_clip_area ();
	}

	public Gth.Image? rotate_image (Gth.Image image, Cancellable cancellable) {
		var rotated = image.rotate (_angle, _background, RotateFilter.BICUBIC, cancellable);
		Gth.Image cropped = null;
		switch (_rotated_size) {
		case RotatedSize.ORIGINAL:
			Graphene.Rect crop = {
				{
					float.max (((float) rotated.get_width () - image_box.size.width) / 2, 0),
					float.max (((float) rotated.get_height () - image_box.size.height) / 2, 0),
				},
				image_box.size
			};
			cropped = rotated.get_subimage (
				(uint) crop.origin.x, (uint) crop.origin.y,
				(uint) crop.size.width, (uint) crop.size.height);
			break;

		case RotatedSize.BOUNDING_BOX:
			cropped = rotated;
			break;

		case RotatedSize.CROP_BORDERS:
			var crop = get_cropping_region (_angle, crop_p1);
			cropped = rotated.get_subimage (
				(uint) crop.origin.x, (uint) crop.origin.y,
				(uint) crop.size.width, (uint) crop.size.height);
			break;
		}
		return cropped;
	}

	public void on_snapshot (Gtk.Snapshot snapshot) {
		snapshot.push_clip (_view.viewport);

		// Background

		Graphene.Rect view_box = { { 0, 0 }, _view.viewport.size };
		snapshot.append_color (_background, view_box);

		// Rotated image

		snapshot.save ();
		var x = _view.texture_box.origin.x;
		var y = _view.texture_box.origin.y;
		var w = _view.texture_box.size.width;
		var h = _view.texture_box.size.height;
		var transform = new Gsk.Transform ()
			.translate ({ x + w / 2, y + h / 2 })
			.rotate (_angle)
			.translate ({ -(x + w / 2), -(y + h / 2) });
		snapshot.transform (transform);
		_view.snapshot_image (snapshot);
		snapshot.restore ();

		// Image cut

		transform = to_screen_transformation ();
		var transformed_crop_region = transform.transform_bounds (_crop_region);
		_view.snapshot_selection (snapshot, transformed_crop_region);

		snapshot.pop ();
	}

	void update_clip_area () {
		if ((_view == null) || (_view.image == null)) {
			_crop_region = { { 0, 0 }, { 0, 0 } };
			return;
		}

		switch (_rotated_size) {
		case RotatedSize.ORIGINAL:
			_crop_region = image_box;
			break;

		case RotatedSize.BOUNDING_BOX:
			_crop_region = get_rotated_bounding_box ();
			break;

		case RotatedSize.CROP_BORDERS:
			update_cropping_parameters (_angle);
			move_crop_region (crop_p1);
			break;
		}
	}

	void move_crop_region (double p1) {
		if (_view.image == null) {
			return;
		}
		crop_p1 = p1;
		_crop_region = get_cropping_region (_angle, crop_p1);
		var bounding_box = get_rotated_bounding_box ();
		_crop_region = {
			{
				_crop_region.origin.x + bounding_box.origin.x,
				_crop_region.origin.y + bounding_box.origin.y
			},
			_crop_region.size
		};
	}

	Gsk.Transform get_rotation (float x = 0, float y = 0) {
		return new Gsk.Transform ()
			.translate ({ x + _center.x, y + _center.y })
			.rotate (_angle)
			.translate ({ -(x + _center.x), -(y + _center.y) });
	}

	Graphene.Rect get_rotated_bounding_box () {
		var rotation = get_rotation ();
		return rotation.transform_bounds (image_box);
	}

	float deg_to_rad (float value) {
		return (float) (value / 180 * Math.PI);
	}

	float rad_to_deg (float value) {
		return (float) (value * 180 / Math.PI);
	}

	void update_cropping_parameters (float angle) {
		if ((_view == null) || (_view.image == null)) {
			crop_p1_plus_p2 = 0;
			crop_p_min = 0;
			return;
		}
		if (angle < -90) {
			angle += 180;
		}
		else if (angle > 90) {
			angle -= 180;
		}
		var angle_rad = (double) deg_to_rad ((float) Math.fabs ((double) angle));
		var cos_angle = Math.cos (angle_rad);
		var sin_angle = Math.sin (angle_rad);
		var src_width  = (float) _view.image.get_width () - 1;
		var src_height = (float) _view.image.get_height () - 1;

		if (src_width > src_height) {
			var t1 = cos_angle * src_width - sin_angle * src_height;
			var t2 = sin_angle * src_width + cos_angle * src_height;

			crop_p1_plus_p2  = 1.0 + (t1 * src_height) / (t2 * src_width);
			crop_p_min = src_height / src_width * sin_angle * cos_angle + (crop_p1_plus_p2 - 1) * cos_angle * cos_angle;
		}
		else {
			var t1 = cos_angle * src_height - sin_angle * src_width;
			var t2 = sin_angle * src_height + cos_angle * src_width;

			crop_p1_plus_p2 = 1.0 + (t1 * src_width) / (t2 * src_height);
			crop_p_min = src_width / src_height * sin_angle * cos_angle + (crop_p1_plus_p2 - 1) * cos_angle * cos_angle;
		}
		// stdout.printf ("> min: %f, max: %f, sum: %f\n", crop_p_min, crop_p1_plus_p2 - crop_p_min, crop_p1_plus_p2);
		// This centers the cropping region in the middle of the rotated image.
		crop_p1 = crop_p1_plus_p2 / 2.0;
		changed_position_parameters ();
	}

	Graphene.Rect get_cropping_region (float angle, double p1) {
		if ((_view == null) || (_view.image == null)) {
			return { { 0, 0 }, { 0, 0 } };
		}
		if (angle < -90) {
			angle += 180;
		}
		else if (angle > 90) {
			angle -= 180;
		}
		var angle_rad = (double) deg_to_rad ((float) Math.fabs ((double) angle));
		var cos_angle = Math.cos (angle_rad);
		var sin_angle = Math.sin (angle_rad);
		var src_width  = (float) _view.image.get_width () - 1;
		var src_height = (float) _view.image.get_height () - 1;

		var p2 = crop_p1_plus_p2 - p1;
		p1 = p1.clamp (0.0, 1.0);
		p2 = p2.clamp (0.0, 1.0);
		// stdout.printf ("> crop_p1_plus_p2: %f\n", crop_p1_plus_p2);
		// stdout.printf ("  p1: %f\n", p1);
		// stdout.printf ("  p2: %f\n", p2);

		if (angle < 0) {
			// This is to make the p1/p2 sliders behavour more user friendly.
			var t = p1;
			p1 = p2;
			p2 = t;
		}

		double xx1, yy1, xx2, yy2;
		if (src_width > src_height) {
			xx1 = p1 * src_width * cos_angle + src_height * sin_angle;
			yy1 = p1 * src_width * sin_angle;
			xx2 = (1 - p2) * src_width * cos_angle;
			yy2 = (1 - p2) * src_width * sin_angle + src_height * cos_angle;
		}
		else {
			xx1 = p1       * src_height * sin_angle;
			yy1 = (1 - p1) * src_height * cos_angle;
			xx2 = (1 - p2) * src_height * sin_angle + src_width * cos_angle;
			yy2 = p2       * src_height * cos_angle + src_width * sin_angle;
		}

		if (angle < 0) {
			var new_width = (cos_angle * src_width) + (sin_angle * src_height);
			xx1 = new_width - xx1;
			xx2 = new_width - xx2;
		}

		var x = Math.round (Math.fmin (xx1, xx2));
		var y = Math.round (Math.fmin (yy1, yy2));
		var width  = Math.round (Math.fmax (xx1, xx2)) - x + 1;
		var height = Math.round (Math.fmax (yy1, yy2)) - y + 1;
		return { { (float) x, (float) y }, { (float) width, (float) height } };
	}

	Gsk.Transform to_screen_transformation () {
		return new Gsk.Transform ()
			.translate ({
				(_view.viewport.size.width - ((float) _view.image.get_width () * _view.zoom)) / 2,
				(_view.viewport.size.height - ((float) _view.image.get_height () * _view.zoom)) / 2
			})
			.scale (_view.zoom, _view.zoom);
	}

	void on_dragging (float x, float y) {
		var pointer_position = PointUtil.point_from_click (x, y);
		var center_on_screen = to_screen_transformation ().transform_point (_center);
		var angle1 = PointUtil.get_angle (center_on_screen, drag_start);
		var angle2 = PointUtil.get_angle (center_on_screen, pointer_position);
		var new_angle = (double) angle_before_dragging + (angle2 - angle1);
		if (new_angle < -Math.PI) {
			new_angle += Math.PI * 2;
		}
		if (new_angle > Math.PI) {
			new_angle -= Math.PI * 2;
		}
		radiants = (float) new_angle;
	}

	public double crop_p1_plus_p2;
	public double crop_p_min;
	public double crop_p1;

	void update_image_box () {
		if ((_view != null) && (_view.image != null)) {
			image_box = {
				{ 0, 0 },
				{ _view.image.get_width (), _view.image.get_height () }
			};
			_center = { (float) image_box.size.width / 2, (float) image_box.size.height / 2 };
		}
		else {
			image_box = {
				{ 0, 0 },
				{ 0, 0 }
			};
			_center = { 0, 0 };
		}
		_crop_region = image_box;
	}

	construct {
		_view = null;
		_angle = 0f;
		_crop_region = { { 0, 0 }, { 0, 0 } };
		background = { 0, 0, 0, 1 };
		view_notify_image_id = 0;

		drag_events = new Gtk.GestureDrag ();
		drag_events.drag_begin.connect ((x, y) => {
			drag_start = PointUtil.point_from_click (x, y);
			angle_before_dragging = deg_to_rad (_angle);
			prev_cursor = _view.cursor;
			_view.cursor = new Gdk.Cursor.from_name ("grabbing", null);
		});
		drag_events.drag_end.connect ((x, y) => {
			_view.cursor = prev_cursor;
		});
		drag_events.drag_update.connect ((local_drag_events, ofs_x, ofs_y) => {
			double start_x, start_y;
			local_drag_events.get_start_point (out start_x, out start_y);
			on_dragging ((float) (start_x + ofs_x), (float) (start_y + ofs_y));
		});
	}

	Graphene.Rect image_box;
	Graphene.Point _center;
	float _angle;
	RotatedSize _rotated_size;
	Graphene.Rect _crop_region;
	ImageView _view;
	Gdk.Cursor prev_cursor;
	Graphene.Point drag_start;
	float angle_before_dragging;
	Gdk.RGBA _background;
	ulong view_notify_image_id;
	Gtk.GestureDrag drag_events;
}
