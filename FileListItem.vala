public class Gth.FileListItem : Gtk.Box {
	public unowned string[] attributes_v;
	public FileData file_data;

	public FileListItem (Gth.Browser _browser, string[] _attributes_v) {
		browser = _browser;
		attributes_v = _attributes_v;
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

		labels.visible = attributes_v.length > 0;
		first_label.visible = attributes_v.length > 0;
		second_label.visible = attributes_v.length > 1;

		var click_events = new Gtk.GestureClick ();
		click_events.set_button (Gdk.BUTTON_SECONDARY);
		click_events.pressed.connect ((n_press, x, y) => {
			browser.open_file_context_menu (this, (int) x, (int) y);
		});
		add_controller (click_events);
	}

	public void bind (FileData _file_data) {
		file_data = _file_data;
		if (size != browser.thumbnail_size) {
			set_size (browser.thumbnail_size);
		}
		if (attributes_v.length > 0) {
			first_label.set_text (file_data.get_attribute_as_string (attributes_v[0]));
		}
		if (attributes_v.length > 1) {
			second_label.set_text (file_data.get_attribute_as_string (attributes_v[1]));
		}
		thumbnail.bind (file_data);
	}

	public void set_size (int _size) {
		if (_size == size)
			return;
		size = _size;
		thumbnail.size = size;
		first_label.set_size_request (size, -1);
		second_label.set_size_request (size, -1);
	}

	public void unbind () {
		thumbnail.unbind ();
		first_label.set_text ("");
		second_label.set_text ("");

	}

	weak Gth.Browser browser;
	Gth.Thumbnail thumbnail;
	Gtk.Inscription first_label;
	Gtk.Inscription second_label;
	int size;

	const int V_SPACING = 6;
}
