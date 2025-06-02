public class Gth.FileListItem : Gtk.Box {
	int size;
	const int V_SPACING = 6;

	public FileListItem (int _size) {
		size = _size;
		orientation = Gtk.Orientation.VERTICAL;
		halign = Gtk.Align.FILL;
		spacing = V_SPACING;
		//add_css_class ("card");
		add_css_class ("thumbnail-card");

		var fixed_size = new Gtk.Box (Gtk.Orientation.HORIZONTAL, 0);
		fixed_size.width_request = size;
		fixed_size.height_request = size;
		append (fixed_size);

		preview = new Gtk.Picture ();
		preview.hexpand = true;
		preview.halign = Gtk.Align.CENTER;
		preview.valign = Gtk.Align.CENTER;
		preview.visible = false;
		preview.can_shrink = false;
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
		append (labels);

		first_label = new Gtk.Label ("");
		first_label.ellipsize = Pango.EllipsizeMode.MIDDLE;
		first_label.halign = Gtk.Align.START;
		labels.append (first_label);

		second_label = new Gtk.Label ("");
		second_label.ellipsize = Pango.EllipsizeMode.MIDDLE;
		second_label.halign = Gtk.Align.START;
		second_label.add_css_class ("dim-label");
		labels.append (second_label);
	}

	public void bind (FileData _file_data) {
		file_data = _file_data;
		first_label.set_text (file_data.info.get_display_name ());
		second_label.set_text (file_data.info.get_attribute_string ("gth::file::display-size"));
		update_preview ();
		thumbnail_texture_id = file_data.notify["thumbnail-texture"].connect ((obj) => {
			update_preview ();
		});
	}

	void update_preview () {
		var thumbnail = file_data.thumbnail_texture;
		if (thumbnail != null) {
			preview.paintable = thumbnail;
			preview.visible = true;
			icon.visible = false;
		}
		else {
			var symbolic_icon = file_data.info.get_attribute_object (FileAttribute.STANDARD_SYMBOLIC_ICON) as Icon;
			if (symbolic_icon != null) {
				icon.gicon = symbolic_icon;
			}
			else {
				icon.clear ();
			}
			icon.visible = true;
			preview.visible = false;
		}
	}

	public void unbind () {
		if (thumbnail_texture_id != 0) {
			file_data.disconnect (thumbnail_texture_id);
			thumbnail_texture_id = 0;
		}
		first_label.set_text ("");
		second_label.set_text ("");
		preview.paintable = null;
	}

	ulong thumbnail_texture_id;
	FileData file_data;
	Gtk.Picture preview;
	Gtk.Image icon;
	Gtk.Label first_label;
	Gtk.Label second_label;
}
