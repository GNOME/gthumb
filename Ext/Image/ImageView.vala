public class Gth.ImageView : Gtk.Widget {
	public Gth.Image image {
		get { return _image; }
		set {
			if (value == _image)
				return;
			_image = value;
			switch (_default_zoom_type) {
			case ZoomType.NATURAL_SIZE:
				_zoom = 1f;
				break;
			case ZoomType.KEEP_PREVIOUS:
				// void
				break;
			default:
				_zoom_type = _default_zoom_type;
				break;
			}
			queue_resize ();
		}
	}

	public float zoom {
		get { return _zoom; }
		set { _zoom = value; }
	}

	public signal void resized ();

	public override void measure (Gtk.Orientation orientation, int for_size, out int minimum, out int natural, out int minimum_baseline, out int natural_baseline) {
		minimum = 0;
		natural = 0;
		natural_baseline = -1;
		minimum_baseline = -1;
	}

	public override void size_allocate (int width, int height, int baseline) {
		viewport.size = { width, height };
		if (_image == null) {
			return;
		}
		if (_zoom_type.fit_allocated_size ()) {
			set_zoom_centered (get_zoom_for_allocation (width, height));
		}
		else {
			// Keep the current zoom.
			update_texture_box ();
			update_image_box ();
			resized ();
		}
	}

	public override void snapshot (Gtk.Snapshot snapshot) {
		if (_image == null)
			return;
		//Util.print_rectangle ("> image_box:", image_box);
		//Util.print_rectangle ("> texture_box:", texture_box);
		snapshot.save ();
		var texture = _image.get_texture_from_point ((uint) image_box.origin.x, (uint) image_box.origin.y);
		snapshot.append_scaled_texture (texture, Gsk.ScalingFilter.NEAREST, texture_box);
		snapshot.restore ();
	}

	float get_zoom_for_allocation (int width, int height) {
		if (image == null) {
			return _zoom;
		}
		float new_zoom = _zoom;
		uint natural_width, natural_height;
		_image.get_natural_size (out natural_width, out natural_height);
		switch (_zoom_type) {
		case ZoomType.FIT_SIZE:
			new_zoom = Util.get_zoom_to_fit_surface (natural_width, natural_height, width, height);
			/*stdout.printf ("[%u,%u] in [%d,%d] -> %f\n",
				natural_width, natural_height, width, height,
				new_zoom);*/
			break;

		case ZoomType.FIT_SIZE_IF_LARGER:
			if ((width < natural_width) || (height < natural_height)) {
				new_zoom = Util.get_zoom_to_fit_surface (natural_width, natural_height, width, height);
			}
			else {
				new_zoom = 1f;
			}
			break;

		case ZoomType.FIT_WIDTH:
			new_zoom = Util.get_zoom_to_fit_length (natural_width, width);
			break;

		case ZoomType.FIT_WIDTH_IF_LARGER:
			if (width < natural_width) {
				new_zoom = Util.get_zoom_to_fit_length (natural_width, width);
			}
			else {
				new_zoom = 1f;
			}
			break;

		case ZoomType.FIT_HEIGHT:
			new_zoom = Util.get_zoom_to_fit_length (natural_height, height);
			break;

		case ZoomType.FIT_HEIGHT_IF_LARGER:
			if (height < natural_height) {
				new_zoom = Util.get_zoom_to_fit_length (natural_height, height);
			}
			else {
				new_zoom = 1f;
			}
			break;

		default:
			break;
		}
		return new_zoom;
	}

	inline void set_zoom_centered (float new_zoom) {
		set_zoom_centered_at (new_zoom,
			(int) viewport.size.width / 2,
			(int) viewport.size.height / 2);
	}

	void set_zoom_centered_at (float new_zoom, int center_x, int center_y) {
		_zoom = new_zoom;
		center_image_area ();
		update_texture_box ();
		update_image_box ();
		resized ();
	}

	void center_image_area () {
		float zoomed_width, zoomed_height;
		get_zoomed_size_for_zoom (_zoom, out zoomed_width, out zoomed_height);
		image_box = { {
				Util.center_content (viewport.size.width, zoomed_width),
				Util.center_content (viewport.size.height, zoomed_height)
			}, {
				zoomed_width,
				zoomed_height
			}
		};
	}

	void update_texture_box () {
		float zoomed_width, zoomed_height;
		get_zoomed_size_for_zoom (_zoom, out zoomed_width, out zoomed_height);
		var texture_width = (uint) zoomed_width;
		var texture_height = (uint) zoomed_height;
		Lib.scale_keeping_ratio (ref texture_width, ref texture_height, (uint) viewport.size.width, (uint) viewport.size.height);
		texture_box = { {
				Util.center_content (viewport.size.width, texture_width),
				Util.center_content (viewport.size.height, texture_height)
			}, {
				texture_width,
				texture_height
			}
		};
	}

	void update_image_box () {
		float zoomed_width, zoomed_height;
		get_zoomed_size_for_zoom (_zoom, out zoomed_width, out zoomed_height);
		uint natural_width, natural_height;
		_image.get_natural_size (out natural_width, out natural_height);
		var fit_width = (zoomed_width <= viewport.size.width);
		var fit_height = (zoomed_height <= viewport.size.height);
		image_box = { {
				fit_width ? 0 : (viewport.origin.x / _zoom).clamp (0, natural_width),
				fit_height ? 0 : (viewport.origin.y / _zoom).clamp (0, natural_width),
			}, {
				fit_width ? natural_width : (viewport.size.width / _zoom).clamp (0, natural_width),
				fit_height ? natural_height : (viewport.size.height / _zoom).clamp (0, natural_height),
			}
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

	construct {
		_image = null;
		_zoom_type = ZoomType.FIT_SIZE_IF_LARGER;
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
}
