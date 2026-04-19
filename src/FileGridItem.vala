public class Gth.FileGridItem : Gtk.Box {
	public FileData file_data;
	public weak Gth.FileGrid file_grid;

	public FileGridItem (Gth.FileGrid _file_grid) {
		file_grid = _file_grid;
		attributes_v = file_grid.thumbnail_attributes_v;
		size = 0;
		orientation = Gtk.Orientation.VERTICAL;
		halign = Gtk.Align.CENTER;
		spacing = V_SPACING;
		add_css_class ("thumbnail-card");

		thumbnail = new Gth.Thumbnail ();
		append (thumbnail);

		var labels = new Gtk.Box (Gtk.Orientation.VERTICAL, V_SPACING);
		labels.halign = Gtk.Align.CENTER;
		append (labels);

		first_label = new Gtk.Inscription ("");
		first_label.text_overflow = Gtk.InscriptionOverflow.ELLIPSIZE_MIDDLE;
		first_label.halign = Gtk.Align.START;
		labels.append (first_label);

		second_label = new Gtk.Inscription ("");
		second_label.text_overflow = Gtk.InscriptionOverflow.ELLIPSIZE_MIDDLE;
		second_label.halign = Gtk.Align.START;
		second_label.add_css_class ("dimmed");
		labels.append (second_label);

		third_label = new Gtk.Inscription ("");
		third_label.text_overflow = Gtk.InscriptionOverflow.ELLIPSIZE_MIDDLE;
		third_label.halign = Gtk.Align.START;
		third_label.add_css_class ("dimmed");
		labels.append (third_label);

		labels.visible = attributes_v.length > 0;
		first_label.visible = attributes_v.length > 0;
		second_label.visible = attributes_v.length > 1;
		third_label.visible = attributes_v.length > 2;
	}

	public void set_size (uint _size) {
		if (_size == size) {
			return;
		}
		size = _size;
		thumbnail.size = size;
		first_label.set_size_request ((int) size, -1);
		second_label.set_size_request ((int) size, -1);
		third_label.set_size_request ((int) size, -1);
	}

	public void bind (FileData _file_data) {
		file_data = _file_data;
		if (size != file_grid.thumbnail_size) {
			set_size (file_grid.thumbnail_size);
		}

		update_attributes ();
		file_renamed_id = file_data.info_changed.connect_after (() => {
			update_attributes ();
		});
		thumbnail.bind (file_data);
	}

	public void unbind () {
		thumbnail.unbind ();
		file_data.disconnect (file_renamed_id);
		first_label.set_text ("");
		second_label.set_text ("");
		third_label.set_text ("");
		tooltip_text = null;
	}

	void update_attributes () {
		if (attributes_v.length > 0) {
			first_label.set_text (file_data.get_attribute_as_string (attributes_v[0]));
		}
		if (attributes_v.length > 1) {
			second_label.set_text (file_data.get_attribute_as_string (attributes_v[1]));
		}
		if (attributes_v.length > 2) {
			third_label.set_text (file_data.get_attribute_as_string (attributes_v[2]));
		}
		if (file_grid.filename_as_tooltip) {
			tooltip_text = file_data.get_display_name ();
		}
	}

	Gth.Thumbnail thumbnail;
	Gtk.Inscription first_label;
	Gtk.Inscription second_label;
	Gtk.Inscription third_label;
	uint size;
	ulong file_renamed_id;
	unowned string[] attributes_v;

	const int V_SPACING = 6;
}
