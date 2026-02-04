public class Gth.PrintLayout : Object {
	public signal void changed ();

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

	public void set_default_output_file (Gtk.PrintSettings settings) {
		var ext = settings.get (Gtk.PRINT_SETTINGS_OUTPUT_FILE_FORMAT);
		if (ext == null) {
			ext = "pdf";
			settings.set (Gtk.PRINT_SETTINGS_OUTPUT_FILE_FORMAT, ext);
		}
		if (images.length == 1) {
			var first_image = images[0];
			if ((first_image.file != null) && first_image.file.has_uri_scheme ("file")) {
				var uri = first_image.file.get_uri () + "." + ext;
				settings.set (Gtk.PRINT_SETTINGS_OUTPUT_URI, uri);
			}
		}
		else {
			var path = Environment.get_user_special_dir (UserDirectory.DOCUMENTS);
			if (path == null) {
				path = Environment.get_home_dir ();
			}
			if (path != null) {
				var uri = Filename.to_uri (path) + "/" + _("Images") + "." + ext;
				settings.set (Gtk.PRINT_SETTINGS_OUTPUT_URI, uri);
			}
		}
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

		settings.set_int (PREF_PRINT_LAYOUT_ROWS, rows);
		settings.set_int (PREF_PRINT_LAYOUT_COLUMNS, columns);
		settings.set_enum (PREF_PRINT_UNIT, default_unit);
	}

	OutputStream stream = null;

	Cairo.Status write_to_stream (uchar[] data) {
		if (stream == null) {
			return Cairo.Status.NULL_POINTER;
		}
		try {
			stream.write (data);
		}
		catch (Error error) {
			// TODO write_error = error;
			stdout.printf ("> ERROR: %s\n", error.message);
			return Cairo.Status.WRITE_ERROR;
		}
		return Cairo.Status.SUCCESS;
	}

	public void print_to_stream (OutputStream _stream, Cancellable cancellable) throws Error {
		stream = _stream;
		printing = true;

		var dpi = print_settings.get_resolution ();
		var page_width = page_setup.get_page_width (Gtk.Unit.INCH) * dpi;
		var page_height = page_setup.get_page_height (Gtk.Unit.INCH) * dpi;
		update_layout (page_width, page_height);

		var paper_width = page_setup.get_paper_width (Gtk.Unit.INCH) * dpi;
		var paper_height = page_setup.get_paper_height (Gtk.Unit.INCH) * dpi;
		var surface = new Cairo.PdfSurface.for_stream (write_to_stream, paper_width, paper_height);
		var cairo_context = new Cairo.Context (surface);

		var left_margin = page_setup.get_left_margin (Gtk.Unit.INCH) * dpi;
		var top_margin = page_setup.get_top_margin (Gtk.Unit.INCH) * dpi;
		cairo_context.translate (left_margin, top_margin);

		for (var page = 1; page <= total_pages; page++) {
			print_page (cairo_context, page, cancellable);
			if (cancellable.is_cancelled ()) {
				throw new IOError.CANCELLED ("Cancelled");
			}
		}

		surface.finish ();
		printing = false;
		stream = null;
	}

	void print_page (Cairo.Context cairo_context, int page_n, Cancellable cancellable) throws Error {
		// TODO: update header and footer texts
		stdout.printf ("> page: %d\n", page_n);
		update_current_page_layout (page_n);
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
			var scale_factor = (double) print_width / image.image.width;

			Cairo.Surface surface;
			if (image.image.get_is_scalable ()) {
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

	public void changed_layout () {
		foreach (unowned var image in images) {
			image.changed_layout ();
		}
		update ();
	}

	public void update () {
		update_layout ();
		update_current_page_layout ();
		changed ();
	}

	public void update_layout (double page_width = 0, double page_height = 0) {
		if (page_setup == null) {
			return;
		}
		if ((page_width <= 0) || (page_height <= 0)) {
			page_width = page_setup.get_page_width (Gtk.Unit.MM);
			page_height = page_setup.get_page_height (Gtk.Unit.MM);
		}

		var height_changed = false;

		// TODO: update header and footer box

		if (!printing && height_changed) {
			foreach (unowned var image in images) {
				image.changed_layout ();
			}
		}

		x_padding = (float) (page_width / 40);
		y_padding = (float) (page_height / 40);

		max_image_width = (float) ((page_width - ((columns - 1) * x_padding)) / columns);
		max_image_height = (float) ((page_height - ((rows - 1) * y_padding)) / rows);

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

	public void update_current_page_layout (int page = 0, int margin_left = 0, int margin_top = 0) {
		if (page == 0) {
			page = _current_page;
		}
		foreach (var image in images) {
			if (image.page != page) {
				continue;
			}
			update_image_layout (image);
		}
	}

	public void update_image_layout (PrintImageLayout image, int margin_left = 0, int margin_top = 0) {
		var top_margin = (header_box.size.height > 0) ? header_box.size.height + y_padding : 0;
		image.bounding_box = {
			{
				(image.column - 1) * (max_image_width + x_padding),
				(image.row - 1) * (max_image_height + y_padding) + top_margin
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

		image.image_box.origin = {
			image.bounding_box.origin.x + (max_x * image.transform.x),
			image.bounding_box.origin.y + (max_y * image.transform.y),
		};

		// Check the limits

		if (image.image_box.origin.x - image.bounding_box.origin.x + image.image_box.size.width > image.bounding_box.size.width) {
			image.image_box.origin.x = image.bounding_box.origin.x + image.bounding_box.size.width - image.image_box.size.width;
			image.transform.x = (image.image_box.origin.x - image.bounding_box.origin.x) / max_image_width;
		}

		if (image.image_box.origin.y - image.bounding_box.origin.y + image.image_box.size.height > image.bounding_box.size.height) {
			image.image_box.origin.y = image.bounding_box.origin.y + image.bounding_box.size.height - image.image_box.size.height;
			image.transform.y = (image.image_box.origin.y - image.bounding_box.origin.y) / max_image_height;
		}

		// TODO: Comment_box
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
		changed_layout ();
	}

	Graphene.Rect header_box;
	Graphene.Rect footer_box;
	float x_padding;
	float y_padding;
	float max_image_width;
	float max_image_height;
	int _current_page;
	GLib.Settings settings;
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
			recenter ();
		}
		get {
			return _zoom;
		}
	}
	public Graphene.Rect image_box;
	public Orientation orientation {
		set {
			_orientation = value;
			changed_layout ();
		}
		get {
			return _orientation;
		}
	}

	public PrintImageLayout (File _file, Image _image) {
		file = _file;
		image = _image;
		scaled_preview = null;
		zoom = 1;
		orientation = Orientation.UP;
	}

	Image scaled_preview;
	Image rotated_preview;

	public void changed_layout () {
		recenter ();
		rotated_preview = null;
	}

	void recenter () {
		transform = { 0.5f, 0.5f };
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
