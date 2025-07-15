public class Gth.FileListItem : Gtk.Box {
	public unowned string[] attributes_v;
	public FileData file_data;

	public FileListItem (Gth.Browser _browser, int _size, string[] _attributes_v) {
		browser = _browser;
		size = _size;
		attributes_v = _attributes_v;
		orientation = Gtk.Orientation.VERTICAL;
		halign = Gtk.Align.CENTER;
		spacing = V_SPACING;
		add_css_class ("thumbnail-card");

		var fixed_size = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
		fixed_size.width_request = size;
		fixed_size.height_request = size;
		//fixed_size.halign = Gtk.Align.CENTER;
		append (fixed_size);

		preview = new Gtk.Picture ();
		preview.hexpand = true;
		preview.halign = Gtk.Align.CENTER;
		preview.valign = Gtk.Align.CENTER;
		preview.visible = false;
		preview.can_shrink = false;
		preview.content_fit = Gtk.ContentFit.SCALE_DOWN;
		fixed_size.append (preview);

		icon = new Gtk.Image ();
		icon.hexpand = true;
		icon.halign = Gtk.Align.CENTER;
		icon.valign = Gtk.Align.CENTER;
		icon.visible = false;
		icon.pixel_size = size / 2;
		icon.add_css_class ("thumbnail-icon");
		fixed_size.append (icon);

		var labels = new Gtk.Box (Gtk.Orientation.VERTICAL, V_SPACING);
		labels.halign = Gtk.Align.CENTER;
		append (labels);

		first_label = new Gtk.Inscription ("");
		first_label.set_size_request (size, -1);
		first_label.text_overflow = Gtk.InscriptionOverflow.ELLIPSIZE_MIDDLE;
		first_label.halign = Gtk.Align.START;
		labels.append (first_label);

		second_label = new Gtk.Inscription ("");
		second_label.set_size_request (size, -1);
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
		if (attributes_v.length > 0) {
			first_label.set_text (file_data.get_attribute_as_string (attributes_v[0]));
		}
		if (attributes_v.length > 1) {
			second_label.set_text (file_data.get_attribute_as_string (attributes_v[1]));
		}
		update_preview ();
		thumbnail_texture_id = file_data.notify["thumbnail-texture"].connect ((obj) => {
			update_preview ();
		});
		thumbnail_state_id = file_data.notify["thumbnail-state"].connect ((obj) => {
			update_preview ();
		});
	}

	void update_preview () {
		switch (file_data.thumbnail_state) {
		case ThumbnailState.ICON:
			icon.gicon = file_data.get_symbolic_icon ();
			icon.visible = true;
			preview.visible = false;
			break;

		case ThumbnailState.LOADING:
			icon.gicon = Util.get_themed_icon ("content-loading-symbolic") as Icon;
			icon.visible = true;
			preview.visible = false;
			break;

		case ThumbnailState.BROKEN:
			//icon.gicon = Util.get_themed_icon ("thumbnail-error-symbolic") as Icon;
			icon.gicon = file_data.get_symbolic_icon ();
			icon.visible = true;
			preview.visible = false;
			break;

		case ThumbnailState.LOADED:
			preview.paintable = file_data.thumbnail_texture;
			preview.visible = true;
			icon.visible = false;
			break;
		}
	}

	public void unbind () {
		if (thumbnail_texture_id != 0) {
			file_data.disconnect (thumbnail_texture_id);
			thumbnail_texture_id = 0;
		}
		if (thumbnail_state_id != 0) {
			file_data.disconnect (thumbnail_state_id);
			thumbnail_state_id = 0;
		}
		first_label.set_text ("");
		second_label.set_text ("");
		preview.paintable = null;
	}

	int size;
	const int V_SPACING = 6;
	weak Gth.Browser browser;
	ulong thumbnail_texture_id;
	ulong thumbnail_state_id;
	Gtk.Picture preview;
	Gtk.Image icon;
	Gtk.Inscription first_label;
	Gtk.Inscription second_label;
}
