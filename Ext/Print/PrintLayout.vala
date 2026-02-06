public class Gth.PrintLayout : Object {
	public signal void changed ();
	public signal void image_changed ();

	public GenericArray<PrintImageLayout> images;
	public Gtk.PageSetup page_setup;
	public Gtk.PrintSettings print_settings;
	public Gtk.PrintSetup print_setup;
	public Gth.PrintUnit default_unit;
	public int rows;
	public int columns;
	public int current_page {
		get { return _current_page; }
		set {
			if (value != _current_page) {
				_current_page = value;
				update_current_page_layout ();
				changed ();
			}
		}
	}
	public int total_pages { get; set; }
	public float left_margin;
	public float top_margin;

	bool printing;
	Pango.Layout pango_layout;

	public PrintLayout (GenericArray<PrintImageLayout> _images) {
		images = _images;

		try {
			var file = UserDir.get_config_file (FileIntent.READ, "print_settings");
			print_settings = new Gtk.PrintSettings.from_file (file.get_path ());
		}
		catch (Error error) {
			print_settings = null;
		}

		try {
			var file = UserDir.get_config_file (FileIntent.READ, "page_setup");
			page_setup = new Gtk.PageSetup.from_file (file.get_path ());
		}
		catch (Error error) {
			page_setup = null;
		}

		settings = new GLib.Settings (GTHUMB_PRINT_SCHEMA);

		rows = settings.get_int (PREF_PRINT_LAYOUT_ROWS);
		columns = settings.get_int (PREF_PRINT_LAYOUT_COLUMNS);
		default_unit = (PrintUnit) settings.get_enum (PREF_PRINT_UNIT);
		_current_page = 0;
		total_pages = 0;
	}

	public bool get_page_is_rotated () {
		var orientation = page_setup.get_orientation ();
		return (orientation == Gtk.PageOrientation.LANDSCAPE) || (orientation == Gtk.PageOrientation.REVERSE_LANDSCAPE);
	}

	public void set_horizontal_margin (double value) {
		page_setup.set_left_margin (value, default_unit.to_gtk_unit ());
		page_setup.set_right_margin (value, default_unit.to_gtk_unit ());
	}

	public void set_vertical_margin (double value) {
		page_setup.set_top_margin (value, default_unit.to_gtk_unit ());
		page_setup.set_bottom_margin (value, default_unit.to_gtk_unit ());
	}

	public void set_default_output_file (Gtk.PrintSettings print_settings) {
		var ext = print_settings.get (Gtk.PRINT_SETTINGS_OUTPUT_FILE_FORMAT);
		if (ext == null) {
			ext = "pdf";
			print_settings.set (Gtk.PRINT_SETTINGS_OUTPUT_FILE_FORMAT, ext);
		}
		var path = settings.get_string (PREF_PRINT_OUTPUT_DIR);
		if (Strings.empty (path)) {
			path = Environment.get_user_special_dir (UserDirectory.DOCUMENTS);
		}
		if (Strings.empty (path)) {
			path = Environment.get_home_dir ();
		}
		string basename = null;
		if (images.length == 1) {
			var first_image = images[0];
			if (first_image.file != null) {
				basename = first_image.file.get_basename ();
			}
		}
		if (basename == null) {
			basename = _("Images");
		}
		var uri = Filename.to_uri (path) + "/" + basename + "." + ext;
		print_settings.set (Gtk.PRINT_SETTINGS_OUTPUT_URI, uri);
	}

	public void set_setup (Gtk.PrintSetup _print_setup) {
		print_setup = _print_setup;
		page_setup = print_setup.get_page_setup ();
		print_settings = print_setup.get_print_settings ();
	}

	public void save_settings () {
		try {
			var file = UserDir.get_config_file (FileIntent.WRITE, "page_setup");
			page_setup.to_file (file.get_path ());
		}
		catch (Error error) {
		}

		try {
			var file = UserDir.get_config_file (FileIntent.WRITE, "print_settings");
			print_settings.to_file (file.get_path ());
		}
		catch (Error error) {
		}

		var uri = print_settings.get (Gtk.PRINT_SETTINGS_OUTPUT_URI);
		if (uri != null) {
			try {
				var filename = Filename.from_uri (uri, null);
				var dirname = Path.get_dirname (filename);
				if (dirname != null) {
					settings.set_string (PREF_PRINT_OUTPUT_DIR, dirname);
				}
			}
			catch (Error error) {
			}
		}
		settings.set_int (PREF_PRINT_LAYOUT_ROWS, rows);
		settings.set_int (PREF_PRINT_LAYOUT_COLUMNS, columns);
		settings.set_enum (PREF_PRINT_UNIT, default_unit);
	}

	OutputStream stream = null;
	Error print_error = null;

	Cairo.Status write_to_stream (uchar[] data) {
		if (stream == null) {
			return Cairo.Status.NULL_POINTER;
		}
		try {
			stream.write (data);
		}
		catch (Error error) {
			print_error = error;
			return Cairo.Status.WRITE_ERROR;
		}
		return Cairo.Status.SUCCESS;
	}

	public void print_to_stream (OutputStream _stream, Cancellable cancellable) throws Error {
		stream = _stream;
		printing = true;
		print_error = null;

		var dpi = print_settings.get_resolution ();
		var page_width = (float) page_setup.get_page_width (Gtk.Unit.INCH) * dpi;
		var page_height = (float) page_setup.get_page_height (Gtk.Unit.INCH) * dpi;
		update_layout (page_width, page_height);

		var paper_width = (int) (page_setup.get_paper_width (Gtk.Unit.INCH) * dpi);
		var paper_height = page_setup.get_paper_height (Gtk.Unit.INCH) * dpi;
		var surface = new Cairo.PdfSurface.for_stream (write_to_stream, paper_width, paper_height);
		var cairo_context = new Cairo.Context (surface);

		left_margin = (float) page_setup.get_left_margin (Gtk.Unit.INCH) * dpi;
		top_margin = (float) page_setup.get_top_margin (Gtk.Unit.INCH) * dpi;
		if (get_page_is_rotated ()) {
			var tmp = left_margin;
			left_margin = top_margin;
			top_margin = tmp;
		}

		for (var page = 1; page <= total_pages; page++) {
			print_page (cairo_context, page, cancellable);
			if (cancellable.is_cancelled ()) {
				throw new IOError.CANCELLED ("Cancelled");
			}
		}

		surface.finish ();
		printing = false;
		stream = null;

		if (print_error != null) {
			throw print_error;
		}
	}

	void print_page (Cairo.Context cairo_context, int page_n, Cancellable cancellable) throws Error {
		update_current_page_layout (page_n, false);
		foreach (var image in images) {
			if (image.page != page_n) {
				continue;
			}
			if ((image.image_box.size.width <= 1) || (image.image_box.size.height <= 1)) {
				continue;
			}

			var print_width = (uint) image.image_box.size.width;
			var print_height = (uint) image.image_box.size.height;
			if (image.orientation.changes_dimensions ()) {
				var tmp = print_width;
				print_width = print_height;
				print_height = tmp;
			}

			Cairo.Surface surface;
			if (image.image.get_is_scalable ()) {
				var scale_factor = (double) print_width / image.image.width;
				surface = image.image.get_scaled_texture (scale_factor, 0, 0, print_width, print_height);
				if (image.orientation != Orientation.UP) {
					var scaled = new Image.from_cairo_surface (surface);
					scaled = scaled.apply_transform (image.orientation.to_transform (), cancellable);
					surface = scaled.to_surface ();
				}
			}
			else {
				var scaled = image.image.resize_to (print_width, print_height, ScaleFilter.BEST, cancellable);
				if (scaled == null) {
					throw new IOError.CANCELLED ("Cancelled");
				}
				if (image.orientation != Orientation.UP) {
					scaled = scaled.apply_transform (image.orientation.to_transform (), cancellable);
				}
				surface = scaled.to_surface ();
			}

			cairo_context.save ();
			var matrix = new Cairo.Matrix.identity ();
			matrix.translate (
				-image.image_box.origin.x,
				-image.image_box.origin.y);
			var pattern = new Cairo.Pattern.for_surface (surface);
			pattern.set_matrix (matrix);
			pattern.set_extend (Cairo.Extend.NONE);
			pattern.set_filter (Cairo.Filter.NEAREST);
			cairo_context.set_source (pattern);
			cairo_context.paint ();
			cairo_context.restore ();
		}
		cairo_context.show_page ();
	}

	public void previous_page () {
		if (_current_page > 1) {
			current_page = _current_page - 1;
		}
	}

	public void next_page () {
		if (_current_page < total_pages) {
			current_page = _current_page + 1;
		}
	}

	public void changed_layout (Recenter recenter = Recenter.NONE) {
		foreach (unowned var image in images) {
			image.changed_layout (recenter);
		}
		update ();
	}

	public void update () {
		update_layout ();
		update_current_page_layout ();
		changed ();
	}

	public void update_layout (float page_width = 0, float page_height = 0) {
		if (page_setup == null) {
			return;
		}
		if ((page_width <= 0) || (page_height <= 0)) {
			page_width = (float) page_setup.get_page_width (default_unit.to_gtk_unit ());
			page_height = (float) page_setup.get_page_height (default_unit.to_gtk_unit ());
		}

		if (!printing) {
			foreach (unowned var image in images) {
				image.changed_layout (Recenter.NONE);
			}
		}

		x_padding = page_width / 40;
		y_padding = page_height / 40;

		max_image_width = (page_width - ((columns - 1) * x_padding)) / columns;
		max_image_height = (page_height - ((rows - 1) * y_padding)) / rows;

		total_pages = int.max ((int) Math.ceilf ((float) images.length / (rows * columns)), 1);
		if (current_page > total_pages) {
			current_page = total_pages;
		}
		if (current_page == 0) {
			current_page = 1;
		}

		var page = 1;
		var row = 1;
		var column = 1;
		foreach (var image in images) {
			image.page = page;
			image.row = row;
			image.column = column;

			column++;
			if (column > columns) {
				row++;
				column = 1;
			}
			if (row > rows) {
				page++;
				row = 1;
				column = 1;
			}
		}
	}

	void update_page_margins () {
		left_margin = (float) page_setup.get_left_margin (default_unit.to_gtk_unit ());
		top_margin = (float) page_setup.get_top_margin (default_unit.to_gtk_unit ());
		if (get_page_is_rotated ()) {
			var tmp = left_margin;
			left_margin = top_margin;
			top_margin = tmp;
		}
	}

	public void update_current_page_layout (int page = 0, bool update_margins = true) {
		if (page == 0) {
			page = _current_page;
		}
		if (update_margins) {
			update_page_margins ();
		}
		foreach (var image in images) {
			if (image.page != page) {
				continue;
			}
			update_image_layout (image);
		}
	}

	public void update_image_layout (PrintImageLayout image) {
		image.bounding_box = {
			{
				left_margin + (image.column - 1) * (max_image_width + x_padding),
				top_margin + (image.row - 1) * (max_image_height + y_padding)
			},
			{
				max_image_width,
				max_image_height
			}
		};

		var image_width = image.image.width;
		var image_height = image.image.height;
		if (image.orientation.changes_dimensions ()) {
			var tmp = image_width;
			image_width = image_height;
			image_height = tmp;
		}

		var factor = float.min (max_image_width / image_width, max_image_height / image_height);
		var maximized_width = factor * image_width;
		var maximized_height = factor * image_height;
		image.maximized_box = {
			{
				image.bounding_box.origin.x + ((max_image_width - maximized_width) / 2),
				image.bounding_box.origin.y + ((max_image_height - maximized_height) / 2),
			},
			{
				maximized_width,
				maximized_height
			}
		};

		image.image_box.size = {
			image.maximized_box.size.width * image.zoom,
			image.maximized_box.size.height * image.zoom,
		};

		var max_x = image.bounding_box.size.width - image.image_box.size.width;
		var max_y = image.bounding_box.size.height - image.image_box.size.height;

		switch (image.recenter) {
		case Recenter.RECENTER:
			image.transform.x = 0.5f;
			image.transform.y = 0.5f;
			break;

		case Recenter.RECENTER_X:
			image.transform.x = 0.5f;
			break;

		case Recenter.RECENTER_Y:
			image.transform.y = 0.5f;
			break;

		case Recenter.KEEP_OLD_CENTER:
			if (max_x > 0) {
				var center_x = (image.old_image_box.origin.x - image.bounding_box.origin.x) + (image.old_image_box.size.width - image.image_box.size.width) / 2;
				image.transform.x = (float) (center_x / max_x);
			}
			else {
				image.transform.x = 0;
			}
			if (max_y > 0) {
				var center_y = (image.old_image_box.origin.y - image.bounding_box.origin.y) + (image.old_image_box.size.height - image.image_box.size.height) / 2;
				image.transform.y = (float) (center_y / max_y);
			}
			else {
				image.transform.y = 0;
			}
			break;

		default:
			break;
		}

		image.recenter = Recenter.NONE;
		image.transform.x = image.transform.x.clamp (0, 1);
		image.transform.y = image.transform.y.clamp (0, 1);

		image.image_box.origin = {
			image.bounding_box.origin.x + (image.transform.x * max_x),
			image.bounding_box.origin.y + (image.transform.y * max_y),
		};
	}

	public void set_page_orientation (Gtk.PageOrientation orientation) {
		if (orientation == page_setup.get_orientation ()) {
			return;
		}
		var unit = default_unit.to_gtk_unit ();
		var left_margin = page_setup.get_left_margin (unit);
		var top_margin = page_setup.get_top_margin (unit);
		page_setup.set_orientation (orientation);
		page_setup.set_left_margin (left_margin, unit);
		page_setup.set_right_margin (left_margin, unit);
		page_setup.set_top_margin (top_margin, unit);
		page_setup.set_bottom_margin (top_margin, unit);
		changed_layout (Recenter.RECENTER);
	}

	public void move_image_by (PrintImageLayout image, float dx, float dy) {
		var max_x = image.bounding_box.size.width - image.image_box.size.width;
		if (max_x > 0) {
			image.transform.x += dx / max_x;
		}
		else {
			image.transform.x = 0;
		}
		var max_y = image.bounding_box.size.height - image.image_box.size.height;
		if (max_y > 0) {
			image.transform.y += dy / max_y;
		}
		else {
			image.transform.y = 0;
		}
		update_image_layout (image);
		image_changed ();
	}

	float x_padding;
	float y_padding;
	float max_image_width;
	float max_image_height;
	int _current_page;
	GLib.Settings settings;
}

public enum Gth.Recenter {
	NONE,
	KEEP_OLD_CENTER,
	RECENTER_X,
	RECENTER_Y,
	RECENTER;
}

public class Gth.PrintImageLayout {
	public File file;
	public Image image;
	public int page;
	public int row;
	public int column;
	public Graphene.Rect bounding_box;
	public Graphene.Rect maximized_box;
	public Graphene.Point transform;
	public float zoom {
		set {
			_zoom = value;
			recenter_at_next_update (Recenter.KEEP_OLD_CENTER);
		}
		get {
			return _zoom;
		}
	}
	public Graphene.Rect image_box;
	public Orientation orientation {
		set {
			_orientation = value;
			changed_layout (Recenter.NONE);
		}
		get {
			return _orientation;
		}
	}
	public Recenter recenter;

	public PrintImageLayout (File _file, Image _image) {
		file = _file;
		image = _image;
		scaled_preview = null;
		zoom = 1;
		recenter = Recenter.RECENTER;
		recenter_at_next_update ();
		orientation = Orientation.UP;
	}

	public Graphene.Rect old_image_box;
	Image scaled_preview;
	Image rotated_preview;

	public void changed_layout (Recenter _recenter) {
		recenter_at_next_update (_recenter);
		rotated_preview = null;
	}

	public void recenter_at_next_update (Recenter _recenter = Recenter.RECENTER) {
		if (_recenter < recenter) {
			return;
		}
		recenter = _recenter;
		if (recenter == Recenter.KEEP_OLD_CENTER) {
			old_image_box = image_box;
		}
	}

	public void snapshot (Gtk.Snapshot snapshot, Graphene.Size paper_size, Graphene.Rect image_box) {
		if (scaled_preview == null) {
			if (image.get_is_scalable ()) {
				var scaled_width = (uint) image_box.size.width;
				var scaled_height = (uint) image_box.size.height;
				Lib.scale_keeping_ratio (ref scaled_width, ref scaled_height, (uint) paper_size.width, (uint) paper_size.height, true);
				var image_width = orientation.changes_dimensions () ? image.height : image.width;
				var scale_factor = (double) scaled_width / image_width;
				var texture = image.get_scaled_texture (
					scale_factor,
					0, 0,
					scaled_width,
					scaled_height);
				scaled_preview = new Image.from_cairo_surface (texture);
			}
			else {
				scaled_preview = image.resize_to ((uint) paper_size.width, (uint) paper_size.height, ScaleFilter.GOOD);
			}
		}

		if (rotated_preview == null) {
			if (orientation != Orientation.UP) {
				rotated_preview = scaled_preview.apply_transform (orientation.to_transform ());
			}
			else {
				rotated_preview = scaled_preview;
			}
		}

		if (rotated_preview != null) {
			snapshot.append_texture (rotated_preview.get_texture (), image_box);
		}
		else {
			Gdk.RGBA gray = { 0.75f, 0.75f, 0.75f, 1 };
			snapshot.append_color (gray, image_box);
		}
	}

	Orientation _orientation;
	float _zoom;
}
