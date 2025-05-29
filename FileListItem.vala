public class Gth.FileListItem : Gtk.Box {
	int size;
	const int V_SPACING = 6;

	public FileListItem (int _size) {
		size = _size;
		orientation = Gtk.Orientation.VERTICAL;
		width_request = size;
		halign = Gtk.Align.FILL;
		spacing = V_SPACING;
		//add_css_class ("card");
		add_css_class ("thumbnail-card");

		image = new Gtk.Image ();
		image.pixel_size = size;
		image.width_request = size;
		image.height_request = size;
		image.halign = Gtk.Align.CENTER;
		append (image);

		var labels = new Gtk.Box (Gtk.Orientation.VERTICAL, V_SPACING);
		append (labels);

		first_label = new Gtk.Label ("");
		first_label.ellipsize = Pango.EllipsizeMode.END;
		first_label.halign = Gtk.Align.START;
		labels.append (first_label);

		second_label = new Gtk.Label ("");
		second_label.ellipsize = Pango.EllipsizeMode.END;
		second_label.halign = Gtk.Align.START;
		second_label.add_css_class ("dim-label");
		labels.append (second_label);
	}

	public void bind (FileData file_data) {
		first_label.set_text (file_data.info.get_display_name ());
		second_label.set_text (file_data.info.get_attribute_string ("gth::file::display-size"));
		var thumbnail = file_data.thumbnail_texture;
		if (thumbnail != null) {
			image.paintable = thumbnail;
		}
		else {
			image.clear ();
		}
		thumbnail_binding = file_data.bind_property (
			"thumbnail-texture",
			image,
			"paintable",
			BindingFlags.DEFAULT);
	}

	public void unbind () {
		if (thumbnail_binding != null) {
			thumbnail_binding.unbind ();
			thumbnail_binding = null;
		}
	}

	Binding thumbnail_binding = null;
	Gtk.Image image;
	Gtk.Label first_label;
	Gtk.Label second_label;
}
