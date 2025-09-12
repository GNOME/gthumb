public class Gth.Thumbnail : Gtk.Box {
	construct {
		orientation = Gtk.Orientation.HORIZONTAL;
		spacing = 0;

		preview = new Gtk.Picture ();
		preview.halign = Gtk.Align.CENTER;
		preview.valign = Gtk.Align.CENTER;
		preview.visible = false;
		preview.content_fit = Gtk.ContentFit.SCALE_DOWN;
		append (preview);

		icon = new Gtk.Image ();
		icon.hexpand = true;
		icon.halign = Gtk.Align.CENTER;
		icon.valign = Gtk.Align.CENTER;
		icon.visible = false;
		icon.add_css_class ("thumbnail-icon");
		append (icon);

		file_data = null;
		thumbnail_texture_id = 0;
		thumbnail_state_id = 0;
		_size = 0;
	}

	public uint size {
		get { return _size; }
		set {
			_size = value;
			width_request = (int) _size;
			height_request = (int) _size;
			preview.width_request = (int) _size;
			preview.height_request = (int) _size;
			icon.pixel_size = (int) (_size / 2);
		}
	}

	public void bind (Gth.FileData _file_data) {
		if (_file_data == file_data) {
			return;
		}
		unbind ();
		file_data = _file_data;
		update ();
		thumbnail_texture_id = file_data.notify["thumbnail-texture"].connect ((obj) => {
			update ();
		});
		thumbnail_state_id = file_data.notify["thumbnail-state"].connect ((obj) => {
			update ();
		});
	}

	public void unbind () {
		if (file_data == null) {
			return;
		}
		if (thumbnail_texture_id != 0) {
			file_data.disconnect (thumbnail_texture_id);
			thumbnail_texture_id = 0;
		}
		if (thumbnail_state_id != 0) {
			file_data.disconnect (thumbnail_state_id);
			thumbnail_state_id = 0;
		}
		preview.paintable = null;
		file_data = null;
	}

	void update () {
		if (file_data == null) {
			return;
		}
		switch (file_data.thumbnail_state) {
		case ThumbnailState.ICON:
			icon.gicon = file_data.get_symbolic_icon ();
			icon.visible = true;
			preview.visible = false;
			break;

		case ThumbnailState.LOADING:
			icon.gicon = Util.get_themed_icon ("gth-content-loading-symbolic") as Icon;
			icon.visible = true;
			preview.visible = false;
			break;

		case ThumbnailState.BROKEN:
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

	~Thumbnail () {
		unbind ();
	}

	FileData file_data;
	Gtk.Picture preview;
	Gtk.Image icon;
	ulong thumbnail_texture_id;
	ulong thumbnail_state_id;
	uint _size;
}
