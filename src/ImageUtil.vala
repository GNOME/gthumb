namespace Gth.ImageUtil {
	Gdk.Paintable? get_drag_icon (Gtk.Widget parent, Gth.Image image, out int hot_x, out int hot_y, uint total_files = 1) {
		const int thumbnail_size = 150;
		const int padding = 2;
		const int outer_radius = 8;
		const int inner_radius = 6;
		const int shadow_radius = 7;

		var resized = image;
		if ((image.width > thumbnail_size) || (image.height > thumbnail_size)) {
			resized = image.resize (thumbnail_size, ResizeFlags.DEFAULT, ScaleFilter.GOOD);
		}

		hot_x = (int) (resized.width / 2);
		hot_y = -24; // TODO: cursor height

		var snapshot = new Gtk.Snapshot ();

		// Shadow

		Gsk.Shadow[] shadow = {
			Gsk.Shadow () {
				color = { 0, 0, 0, 0.75f },
				dx = 0,
				dy = 0,
				radius = shadow_radius
			}
		};
		snapshot.push_shadow (shadow);

		// White background

		var background_rect = Graphene.Rect () {
			origin = { shadow_radius, shadow_radius },
			size = { resized.width + (padding * 2), resized.height + (padding * 2) },
		};
		Gdk.RGBA bg_color = { 1, 1, 1, 1 };

		if (total_files > 1) {
			// Second white background

			var second_background_rect = Graphene.Rect () {
				origin = {
					shadow_radius + 4,
					shadow_radius + 4
				},
				size = background_rect.size
			};
			var clip = Gsk.RoundedRect ();
			clip.init_from_rect (second_background_rect, outer_radius + 3);
			snapshot.push_rounded_clip (clip);
			snapshot.append_color (bg_color, second_background_rect);
			snapshot.pop ();

			// Fake texture

			var texture_rect = Graphene.Rect () {
				origin = {
					second_background_rect.origin.x + padding,
					second_background_rect.origin.y + padding
				},
				size = { resized.width, resized.height },
			};
			clip = Gsk.RoundedRect ();
			clip.init_from_rect (texture_rect, inner_radius + 3);
			snapshot.push_rounded_clip (clip);
			Gdk.RGBA texture_color = { 0.3f, 0.3f, 0.3f, 1 };
			snapshot.append_color (texture_color, texture_rect);
			snapshot.pop ();
		}

		var clip = Gsk.RoundedRect ();
		clip.init_from_rect (background_rect, outer_radius);
		snapshot.push_rounded_clip (clip);
		snapshot.append_color (bg_color, background_rect);
		snapshot.pop ();

		snapshot.pop ();

		// Thumbnail

		var texture_rect = Graphene.Rect () {
			origin = {
				background_rect.origin.x + padding,
				background_rect.origin.y + padding
			},
			size = { resized.width, resized.height },
		};
		clip = Gsk.RoundedRect ();
		clip.init_from_rect (texture_rect, inner_radius);
		snapshot.push_rounded_clip (clip);
		snapshot.append_texture (resized.get_texture (), texture_rect);
		snapshot.pop ();

		// Total files

		if (total_files > 1) {
			const int v_padding = 2;
			const int h_padding = 8;
			Gdk.RGBA text_color = { 0, 0, 0, 1 };
			var layout = parent.create_pango_layout ("%u".printf (total_files));
			Pango.Rectangle layout_rect;
			layout.get_extents (null, out layout_rect);
			var text_rect = Graphene.Rect () {
				origin = { 0, 0 },
				size = {
					(layout_rect.width / Pango.SCALE) + (h_padding * 2),
					(layout_rect.height / Pango.SCALE) + (v_padding * 2)
				}
			};

			snapshot.translate ({
				background_rect.size.width - (text_rect.size.width / 2) + 7,
				background_rect.size.height - (text_rect.size.height / 2) - 20
			});

			// Background

			clip = Gsk.RoundedRect ();
			clip.init_from_rect (text_rect, outer_radius);
			snapshot.push_rounded_clip (clip);
			snapshot.append_color (bg_color, text_rect);
			snapshot.pop ();

			// Text

			snapshot.translate ({ h_padding, v_padding });
			snapshot.append_layout (layout, text_color);
		}

		return snapshot.free_to_paintable (null);
	}
}
