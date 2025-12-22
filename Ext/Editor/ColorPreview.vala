public class Gth.ColorPreview : Gtk.Widget {
	public Gdk.RGBA color {
		set {
			_color = value;
			queue_draw ();
		}
		get {
			return _color;
		}
	}

	construct {
		add_css_class ("color-preview");
	}

	public override void measure (Gtk.Orientation orientation, int for_size, out int minimum, out int natural, out int minimum_baseline, out int natural_baseline) {
		if (orientation == Gtk.Orientation.HORIZONTAL) {
			minimum = NATURAL_WIDTH;
			natural = NATURAL_WIDTH;
			natural_baseline = -1;
			minimum_baseline = -1;
		}
		else {
			minimum = NATURAL_HEIGHT;
			natural = NATURAL_HEIGHT;
			natural_baseline = -1;
			minimum_baseline = -1;
		}
	}

	public override void size_allocate (int width, int height, int baseline) {
		viewport = {
			{ PADDING, PADDING },
			{ width - (PADDING * 2), height - (PADDING * 2) }
		};
	}

	public override void snapshot (Gtk.Snapshot snapshot) {
		var background = new Gsk.RoundedRect ();
		background.init_from_rect (viewport, BORDER_RADIUS);
		snapshot.push_rounded_clip (background);

		if (_color.alpha < 1f) {
			var ctx = snapshot.append_cairo (viewport);
			ctx.rectangle (viewport.origin.x, viewport.origin.y,
				viewport.size.width, viewport.size.height);
			var pattern = get_transparency_pattern ();
			var matrix = Cairo.Matrix.identity ();
			matrix.translate (- viewport.origin.x, - viewport.origin.y);
			pattern.set_matrix (matrix);
			ctx.set_source (pattern);
			ctx.fill ();
		}
		snapshot.append_color (_color, viewport);

		// Border
		Gdk.RGBA border_color = { 1.0f, 1.0f, 1.0f, 0.10f };
		float border_width = 2;
		snapshot.append_border (background,
			{ border_width, border_width, border_width, border_width } ,
			{ border_color, border_color, border_color, border_color }
		);

		snapshot.pop ();
	}

	Cairo.Pattern get_transparency_pattern () {
		if (transparency_pattern == null) {
			transparency_pattern = Color.build_transparency_pattern (TRANSP_PATTERN_SIZE);
		}
		return transparency_pattern;
	}

	Graphene.Rect viewport;
	Gdk.RGBA _color;
	static Cairo.Pattern transparency_pattern = null;

	const int TRANSP_PATTERN_SIZE = 14; // pixels
	const int BORDER_RADIUS = 10;
	const int PADDING = 0;
	const int NATURAL_WIDTH = 70; // pixels
	const int NATURAL_HEIGHT = 30; // pixels
}
