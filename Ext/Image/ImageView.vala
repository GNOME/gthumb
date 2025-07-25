public class Gth.ImageView : Gtk.Widget {
	public void set_image (Gth.Image _image) {
		if (_image == image)
			return;
		image = _image;
		queue_resize ();
	}

	public override void measure (Gtk.Orientation orientation, int for_size, out int minimum, out int natural, out int minimum_baseline, out int natural_baseline) {
		minimum = 0;
		natural = 0;
		natural_baseline = -1;
		minimum_baseline = -1;
	}

	public override void size_allocate (int width, int height, int baseline) {
		viewport.size = { width, height };
		if (image != null) {
			var image_width = image.get_width ();
			var image_height = image.get_height ();
			image_box = { { 0, 0 }, { image_width, image_height } };

			var texture_x = 0f;
			var texture_y = 0f;
			var texture_width = image_width;
			var texture_height = image_height;
			Lib.scale_keeping_ratio (ref texture_width, ref texture_height, (uint) viewport.size.width, (uint) viewport.size.height);
			if (texture_width < viewport.size.width) {
				texture_x = (viewport.size.width - texture_width) / 2f;
			}
			if (texture_height < viewport.size.height) {
				texture_y = (viewport.size.height - texture_height) / 2f;
			}
			texture_box = { { texture_x, texture_y }, { texture_width, texture_height } };
		}
	}

	public override void snapshot (Gtk.Snapshot snapshot) {
		if (image == null)
			return;
		snapshot.save ();
		var texture = image.get_texture_from_point ((uint) image_box.origin.x, (uint) image_box.origin.y);
		snapshot.append_scaled_texture (texture, Gsk.ScalingFilter.NEAREST, texture_box);
		snapshot.restore ();
	}

	construct {
		image = null;
	}

	Gth.Image image;
	// viewport.origin: scroll offsets
	// viewport.size: allocation size
	Graphene.Rect viewport;
	// position and size of the texture node
	Graphene.Rect texture_box;
	// visible area of the image
	Graphene.Rect image_box;
}
