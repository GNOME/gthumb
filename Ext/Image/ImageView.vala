public class Gth.ImageView : Gtk.Widget, Gtk.Scrollable {
	public Gth.Image image {
		get { return _image; }
		set {
			if (value == _image)
				return;
			_image = value;
			if (_default_zoom_type != ZoomType.KEEP_PREVIOUS) {
				_zoom_type = _default_zoom_type;
			}
			switch (_zoom_type) {
			case ZoomType.NATURAL_SIZE:
				set_zoom_and_recenter_image (1f);
				break;
			case ZoomType.KEEP_PREVIOUS:
				set_zoom_and_recenter_image (_zoom);
				break;
			default:
				break;
			}
			queue_resize ();
		}
	}

	public float zoom {
		get { return _zoom; }
		set {
			_zoom_type = ZoomType.KEEP_PREVIOUS;
			set_zoom_and_keep_center_visible (value);
			queue_resize ();
		}
	}

	public ZoomType zoom_type {
		get { return _zoom_type; }
		set {
			_zoom_type = value;
			if (_zoom_type == ZoomType.NATURAL_SIZE) {
				set_zoom_and_recenter_image (1f);
			}
			queue_resize ();
		}
	}

	public ZoomType default_zoom_type {
		get { return _default_zoom_type; }
		set {
			_default_zoom_type = value;
		}
	}

	public Gtk.Adjustment hadjustment {
		get { return _hadjustment; }
		set construct {
			_hadjustment = value;
			if (_hadjustment != null)
				_hadjustment.value_changed.connect(() => queue_draw ());
		}
	}

	public Gtk.Adjustment vadjustment {
		get { return _vadjustment; }
		set construct {
			_vadjustment = value;
			if (_vadjustment != null)
				_vadjustment.value_changed.connect(() => queue_draw ());
		}
	}

	public Gtk.ScrollablePolicy hscroll_policy { get; set; }

	public Gtk.ScrollablePolicy vscroll_policy { get; set; }

	public bool get_border (out Gtk.Border border)  {
		border = Gtk.Border();
		return false;
	}

	public signal void resized ();

	public override void measure (Gtk.Orientation orientation, int for_size, out int minimum, out int natural, out int minimum_baseline, out int natural_baseline) {
		minimum = 0;
		natural = 0;
		natural_baseline = -1;
		minimum_baseline = -1;

		float zoomed_width, zoomed_height;
		get_zoomed_size_for_zoom (_zoom, out zoomed_width, out zoomed_height);
		if (orientation == Gtk.Orientation.HORIZONTAL) {
			natural = (int) zoomed_width;
		}
		else {
			natural = (int) zoomed_height;
		}
	}

	public override void size_allocate (int width, int height, int baseline) {
		viewport.size = { width, height };
		if (_image == null) {
			return;
		}
		if (_zoom_type.fit_to_allocation ()) {
			set_zoom_and_recenter_image (get_zoom_for_allocation (width, height));
		}
		update_texture_box ();
		update_image_box ();
		update_adjustments ();
		resized ();
	}

	public override void snapshot (Gtk.Snapshot snapshot) {
		if (_image == null)
			return;
		//Util.print_rectangle ("> image_box:", image_box);
		//Util.print_rectangle ("> texture_box:", texture_box);
		snapshot.save ();
		var texture = _image.get_texture_for_rect (
			(uint) image_box.origin.x,
			(uint) image_box.origin.y,
			(uint) image_box.size.width,
			(uint) image_box.size.height);
		if (texture != null) {
			snapshot.append_scaled_texture (texture, Gsk.ScalingFilter.NEAREST, texture_box);
		}
		snapshot.restore ();
	}

	public void scroll_by (float dx, float dy) {
		set_scroll_offset (_zoom, viewport.origin.x + dx, viewport.origin.y + dy);
		update_texture_box ();
		update_image_box ();
		queue_draw ();
	}

	float get_zoom_for_allocation (int width, int height) {
		if (image == null) {
			return _zoom;
		}
		float new_zoom = _zoom;
		uint natural_width, natural_height;
		_image.get_natural_size (out natural_width, out natural_height);
		switch (_zoom_type) {
		case ZoomType.MAXIMIZE:
			new_zoom = Util.get_zoom_to_fit_surface (natural_width, natural_height, width, height);
			/*stdout.printf ("[%u,%u] in [%d,%d] -> %f\n",
				natural_width, natural_height, width, height,
				new_zoom);*/
			break;

		case ZoomType.MAXIMIZE_IF_LARGER:
			if ((width < natural_width) || (height < natural_height)) {
				new_zoom = Util.get_zoom_to_fit_surface (natural_width, natural_height, width, height);
			}
			else {
				new_zoom = 1f;
			}
			break;

		case ZoomType.MAXIMIZE_WIDTH:
			new_zoom = Util.get_zoom_to_fit_length (natural_width, width);
			break;

		case ZoomType.MAXIMIZE_HEIGHT:
			new_zoom = Util.get_zoom_to_fit_length (natural_height, height);
			break;

		case ZoomType.FILL_SPACE:
			new_zoom = Util.get_zoom_to_fill_surface (natural_width, natural_height, width, height);
			break;

		default:
			break;
		}
		return new_zoom;
	}

	void set_zoom_and_recenter_image (float new_zoom) {
		float zoomed_width, zoomed_height;
		get_zoomed_size_for_zoom (new_zoom, out zoomed_width, out zoomed_height);

		var offset_x = (zoomed_width - viewport.size.width) / 2;
		var offset_y = (zoomed_height - viewport.size.height) / 2;
		set_scroll_offset (new_zoom, offset_x, offset_y);

		_zoom = new_zoom;
	}

	void set_zoom_and_keep_center_visible (float new_zoom) {
		set_zoom_and_center_at (new_zoom, viewport.size.width / 2, viewport.size.height / 2);
	}

	void set_zoom_and_center_at (float new_zoom, float pointer_x, float pointer_y) {
		if ((texture_box.size.width == 0) || (viewport.size.width == 0)) {
			set_scroll_offset (new_zoom, 0, 0);
		}
		else {
			var old_zoom = _zoom;

			// Pixel under the pointer.
			var new_pointer_x = (viewport.origin.x + pointer_x - texture_box.origin.x) / old_zoom * new_zoom;
			var new_pointer_y = (viewport.origin.y + pointer_y - texture_box.origin.y) / old_zoom * new_zoom;

			// Offset to center the pointer.
			var offset_x = new_pointer_x - (viewport.size.width / 2);
			var offset_y = new_pointer_y - (viewport.size.height / 2);
			set_scroll_offset (new_zoom, offset_x, offset_y);
		}
		_zoom = new_zoom;
	}

	void set_scroll_offset (float zoom_level, float x, float y) {
		float zoomed_width, zoomed_height;
		get_zoomed_size_for_zoom (zoom_level, out zoomed_width, out zoomed_height);
		var max_x = float.max (zoomed_width - viewport.size.width, 0);
		var max_y = float.max (zoomed_height - viewport.size.height, 0);
		x = x.clamp (0, max_x);
		y = y.clamp (0, max_y);
		//stdout.printf ("> SCROLL OFFSET: %f,%f\n", x, y);
		viewport.origin = { x, y };
		// TODO: update adjustment values
	}

	void update_texture_box () {
		//stdout.printf ("> update_texture_box\n");
		float zoomed_width, zoomed_height;
		get_zoomed_size_for_zoom (_zoom, out zoomed_width, out zoomed_height);

		float texture_x, texture_width;
		if (viewport.origin.x + zoomed_width > viewport.size.width) {
			texture_x = 0;
			texture_width = viewport.size.width;
		}
		else {
			texture_width = zoomed_width;
			texture_x = Util.center_content (viewport.size.width, zoomed_width);
		}

		float texture_y, texture_height;
		if (viewport.origin.y + zoomed_height > viewport.size.height) {
			texture_y = 0;
			texture_height = viewport.size.height;
		}
		else {
			texture_height = zoomed_height;
			texture_y = Util.center_content (viewport.size.height, zoomed_height);
		}
		texture_box = {
			{ texture_x, texture_y },
			{ texture_width, texture_height }
		};
	}

	// To be called after updating texture_box.
	void update_image_box () {
		//stdout.printf ("> update_image_box\n");
		uint natural_width, natural_height;
		_image.get_natural_size (out natural_width, out natural_height);

		float image_x, image_width;
		if (texture_box.origin.x > 0) {
			image_x = 0;
			image_width = natural_width;
		}
		else {
			image_x = viewport.origin.x / _zoom;
			image_width = viewport.size.width / _zoom;
		}

		float image_y, image_height;
		if (texture_box.origin.y > 0) {
			image_y = 0;
			image_height = natural_height;
		}
		else {
			image_y = viewport.origin.y / _zoom;
			image_height = viewport.size.height / _zoom;
		}

		image_box = {
			{ image_x, image_y },
			{ image_width, image_height }
		};
	}

	void get_zoomed_size_for_zoom (float zoom_level, out float width, out float height) {
		if (_image != null) {
			uint natural_width, natural_height;
			_image.get_natural_size (out natural_width, out natural_height);
			width = zoom_level * natural_width;
			height = zoom_level * natural_height;
		}
		else {
			width = 0f;
			height = 0f;
		}
	}

	void update_adjustments () {
		float zoomed_width, zoomed_height;
		get_zoomed_size_for_zoom (_zoom, out zoomed_width, out zoomed_height);
		if (_vadjustment != null) {
			_vadjustment.set_lower (0);
			var upper = zoomed_height - viewport.size.height;
			_vadjustment.set_upper ((upper > 0) ? upper : 0);
			_vadjustment.set_page_size ((upper > 0) ? viewport.size.height : 0);
		}
		if (_hadjustment != null) {
			_hadjustment.set_lower (0);
			var upper = zoomed_width - viewport.size.width;
			_vadjustment.set_upper ((upper > 0) ? upper : 0);
			_vadjustment.set_page_size ((upper > 0) ? viewport.size.width : 0);
		}
	}

	construct {
		_image = null;
		_zoom_type = ZoomType.MAXIMIZE_IF_LARGER;
		_default_zoom_type = ZoomType.KEEP_PREVIOUS;
		_zoom = 1f;
	}

	Gth.Image _image;
	Gth.ZoomType _zoom_type;
	Gth.ZoomType _default_zoom_type;
	float _zoom;
	// viewport.origin: scroll offsets
	// viewport.size: allocation size
	Graphene.Rect viewport;
	// position and size of the texture node
	Graphene.Rect texture_box;
	// visible area of the image
	Graphene.Rect image_box;
	Gtk.Adjustment _hadjustment;
	Gtk.Adjustment _vadjustment;
}
