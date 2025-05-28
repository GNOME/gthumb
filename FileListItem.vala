public class Gth.FileListItem : Gtk.Box {
	const int V_SPACING = 10;
	const int H_PADDING = 5;
	int size;

	public FileListItem (int _size) {
		size = _size;
		orientation = Gtk.Orientation.VERTICAL;
		spacing = V_SPACING;
		width_request = size;

		image = new Gtk.Image ();
		image.set_pixel_size (size - (V_SPACING * 2));
		append (image);

		label = new Gtk.Label ("");
		label.ellipsize = Pango.EllipsizeMode.END;
		label.halign = Gtk.Align.START;
		label.margin_start = H_PADDING;
		label.margin_end = H_PADDING;
		append (label);
	}

	public void bind (FileData file_data) {
		label.set_text (file_data.info.get_display_name ());
		var thumbnail = file_data.thumbnail_texture;
		if (thumbnail != null) {
			image.paintable = thumbnail;
		}
		else {
			image.set_from_gicon (file_data.info.get_attribute_object (FileAttribute.STANDARD_ICON) as Icon);
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
	Gtk.Label label;
}
