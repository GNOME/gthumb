public class Gth.VideoView : Gtk.Widget {
	public float zoom { get { return _zoom; } }
	public Gth.ZoomType zoom_type {
		get { return _zoom_type; }
		set {
			_zoom_type = value;
			queue_resize ();
		}
	}

	public signal void resized ();

	public void set_paintable (Gdk.Paintable _paintable) {
		unbind_current_paintable ();
		paintable = _paintable;
		invalidate_contents_id = paintable.invalidate_contents.connect (() => queue_draw ());
		invalidate_size_id = paintable.invalidate_size.connect (() => queue_resize ());
		queue_resize ();
	}

	public override void measure (Gtk.Orientation orientation, int for_size, out int minimum, out int natural, out int minimum_baseline, out int natural_baseline) {
		minimum = 0;
		natural = 0;
		natural_baseline = -1;
		minimum_baseline = -1;
	}

	public override void size_allocate (int width, int height, int baseline) {
		if (paintable == null) {
			_zoom = 1.0f;
			return;
		}
		float zoomed_width, zoomed_height;
		_zoom = get_zoom_for_allocation (width, height, out zoomed_width, out zoomed_height);
		texture_box = { {
				Util.center_content (width, zoomed_width),
				Util.center_content (height, zoomed_height)
			}, {
				zoomed_width,
				zoomed_height
			}
		};
		resized ();
	}

	public override void snapshot (Gtk.Snapshot snapshot) {
		if (paintable == null)
			return;
		//Util.print_rectangle ("> texture_box:", texture_box);
		snapshot.save ();
		snapshot.translate (texture_box.origin);
		paintable.snapshot (snapshot, (double) texture_box.size.width, (double) texture_box.size.height);
		snapshot.restore ();
	}

	float get_zoom_for_allocation (int width, int height, out float zoomed_width, out float zoomed_height) {
		var zoom = 1.0f;
		var natural_width = paintable.get_intrinsic_width ();
		var natural_height = paintable.get_intrinsic_height ();
		if (_zoom_type == ZoomType.FIT_SIZE_IF_LARGER) {
			if ((width < natural_width) || (height < natural_height)) {
				zoom = Util.get_zoom_to_fit_surface (natural_width, natural_height, width, height);
			}
			else {
				zoom = 1f;
			}

		}
		else {
			zoom = Util.get_zoom_to_fit_surface (natural_width, natural_height, width, height);
		}
		zoomed_width = zoom * natural_width;
		zoomed_height = zoom * natural_height;
		return zoom;
	}

	void unbind_current_paintable () {
		if (paintable == null) {
			return;
		}
		if (invalidate_contents_id != 0) {
			paintable.disconnect (invalidate_contents_id);
			invalidate_contents_id = 0;
		}
		if (invalidate_size_id != 0) {
			paintable.disconnect (invalidate_size_id);
			invalidate_size_id = 0;
		}
		paintable = null;
	}

	construct {
		paintable = null;
		invalidate_contents_id = 0;
		invalidate_size_id = 0;
		_zoom = 1f;
		_zoom_type = ZoomType.FIT_SIZE_IF_LARGER;
	}

	~VideoView() {
		unbind_current_paintable ();
	}

	Gdk.Paintable paintable;
	ulong invalidate_contents_id;
	ulong invalidate_size_id;
	Graphene.Rect texture_box;
	Gth.ZoomType _zoom_type;
	float _zoom;
}
