public class Gth.Color {
	public enum Component {
		RED = 0,
		GREEN = 1,
		BLUE = 2,
		ALPHA = 3,
	}

	public static Gdk.RGBA get_rgba (float red, float green, float blue, float alpha = 1) {
		return { red, green, blue, alpha };
	}

	public static void get_hsv_from_rgba (Gdk.RGBA rgba, out float hue, out float saturation, out float value, out float alpha) {
		var max = Color.get_max_value (rgba);
		var min = Color.get_min_value (rgba);
		float chroma = max - min;
		float lightness = (float) (max + min) / 2;
		value = max;
		if (chroma == 0) {
			hue = 0;
		}
		else if (value == rgba.red) {
			hue = 60 * (((rgba.green - rgba.blue) / chroma) % 6);
		}
		else if (value == rgba.green) {
			hue = 60 * (((rgba.blue - rgba.red) / chroma) + 2);
		}
		else if (value == rgba.blue) {
			hue = 60 * (((rgba.red - rgba.green) / chroma) + 4);
		}
		saturation = (value != 0) ? chroma / value : 0;
		alpha = rgba.alpha;
	}

	public static Gdk.RGBA get_rgba_from_hsv (float hue, float saturation, float value, float alpha = 1) {
		hue = Color.normalize_hue (hue);
		saturation = saturation.clamp (0, 1);
		value = value.clamp (0, 360);
		float chroma = value * saturation;
		float h = (hue / 60f);
		float x = chroma * (1f - Math.fabsf ((h % 2) - 1));
		float r, g, b;
		if (h < 1) {
			r = chroma;
			g = x;
			b = 0;
		}
		else if (h < 2) {
			r = x;
			g = chroma;
			b = 0;
		}
		else if (h < 3) {
			r = 0;
			g = chroma;
			b = x;
		}
		else if (h < 4) {
			r = 0;
			g = x;
			b = chroma;
		}
		else if (h < 5) {
			r = x;
			g = 0;
			b = chroma;
		}
		else if (h < 6) {
			r = chroma;
			g = 0;
			b = x;
		}
		else {
			r = 0;
			g = 0;
			b = 0;
		}
		float m = value - chroma;
		return {
			(r + m).clamp (0, 1),
			(g + m).clamp (0, 1),
			(b + m).clamp (0, 1),
			alpha
		};
	}

	public static Gdk.RGBA? get_rgba_from_hexcode (string value) {
		var color = new Gdk.RGBA ();
		if (!color.parse (value))
			return null;
		return color;
	}

	public static string? rgba_to_hexcode (Gdk.RGBA? color) {
		if (color == null)
			return null;

		var str = new StringBuilder ("#");

		void add_component (float value) {
			const char[] HEX_DIGIT = {
				'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
				'a', 'b', 'c', 'd', 'e', 'f'
			};
			var ivalue = ((int) Math.roundf (value * 255f)).clamp (0, 255);
			str.append_c (HEX_DIGIT[ivalue / 16]);
			str.append_c (HEX_DIGIT[ivalue % 16]);
		}

		add_component (color.red);
		add_component (color.green);
		add_component (color.blue);
		if (color.alpha < 1)
			add_component (color.alpha);
		return str.str;
	}

	public static bool is_white (Gdk.RGBA? color) {
		return (color.red == 1) && (color.green == 1) && (color.blue == 1);
	}

	public static bool is_black (Gdk.RGBA? color) {
		return (color.red == 0) && (color.green == 0) && (color.blue == 0);
	}

	public static float get_max_value (Gdk.RGBA color) {
		var max = color.red;
		if (color.green > max)
			max = color.green;
		if (color.blue > max)
			max = color.blue;
		return max;
	}

	public static float get_min_value (Gdk.RGBA color) {
		var min = color.red;
		if (color.green < min)
			min = color.green;
		if (color.blue < min)
			min = color.blue;
		return min;
	}

	public static float get_max_saturation (Gdk.RGBA color) {
		var max = Color.get_max_value (color);
		var min = Color.get_min_value (color);
		var range = max - min;
		return (range > 0) ? (max / range) : 1.0f;
	}

	public static float get_hue (Gdk.RGBA color) {
		var max = Color.get_max_value (color);
		var min = Color.get_min_value (color);
		var chroma = max - min;
		if (chroma == 0)
			return 0; // actually undefined
		var h = 0f;
		if (max == color.red)
			h = ((color.green - color.blue) / chroma) % 6;
		else if (max == color.green)
			h = ((color.blue - color.red) / chroma) + 2;
		else if (max == color.blue)
			h = ((color.red - color.green) / chroma) + 4;
		return Color.normalize_hue (h * 60);
	}

	public static float get_saturation (Gdk.RGBA color) {
		var max = Color.get_max_value (color);
		var min = Color.get_min_value (color);
		var chroma = max - min;
		return (max > 0.0) ? (chroma / max) : 0.0f;
	}

	public static float get_brightness (Gdk.RGBA color) {
		return Color.get_max_value (color);
	}

	public static float get_lightness (Gdk.RGBA color) {
		var max = Color.get_max_value (color);
		var min = Color.get_min_value (color);
		var chroma = max - min;
		return max - ((max - min) / 2);
	}

	public static float get_component (Gdk.RGBA color, Component component) {
		var value = 0.0f;
		switch (component) {
		case Component.RED:
			value = color.red;
			break;
		case Component.GREEN:
			value = color.green;
			break;
		case Component.BLUE:
			value = color.blue;
			break;
		case Component.ALPHA:
			value = color.alpha;
			break;
		}
		return value;
	}

	public static float get_delta (Gdk.RGBA a, Gdk.RGBA b) {
		return Math.fabsf (a.red - b.red) +
			Math.fabsf (a.green - b.green) +
			Math.fabsf (a.blue - b.blue);
	}

	// Contrast ratio formula: https://www.w3.org/TR/WCAG20-TECHS/G18.html
	public static float get_contrast_ratio (Gdk.RGBA lighter_color, Gdk.RGBA darker_color) {
		static float relative_luminance (Gdk.RGBA color) {
			static float luminance (float c) {
				return (c <= 0.03928f) ? c / 12.92f : Math.powf (((c + 0.055f) / 1.055f), 2.4f);
			}
			return 0.2126f * luminance (color.red)
				+ 0.7152f * luminance (color.green)
				+ 0.0722f * luminance (color.blue);
		}
		return (relative_luminance (lighter_color) + 0.05f) / (relative_luminance (darker_color) + 0.05f);
	}

	public static float normalize_hue (float hue) {
		if ((hue >= 0) && (hue < 360))
			return hue;
		hue = hue % 360;
		if (hue < 0)
			hue += 360;
		return hue;
	}

	public static Gdk.RGBA change_saturation (Gdk.RGBA color, float saturation, float max = -1.0f) {
		if (max < 0.0f)
			max = Color.get_max_value (color);
		return {
			(max - ((max - color.red) * saturation)).clamp (0.0f, 1.0f),
			(max - ((max - color.green) * saturation)).clamp (0.0f, 1.0f),
			(max - ((max - color.blue) * saturation)).clamp (0.0f, 1.0f),
			1.0f
		};
	}

	public static Gdk.RGBA change_brightness (Gdk.RGBA color, float brightness, float max = -1.0f) {
		if (max < 0.0f)
			max = Color.get_max_value (color);
		return {
			(brightness * (color.red / max)).clamp (0.0f, 1.0f),
			(brightness * (color.green / max)).clamp (0.0f, 1.0f),
			(brightness * (color.blue / max)).clamp (0.0f, 1.0f),
			1.0f
		};
	}

	public static Gdk.RGBA change_component (Gdk.RGBA color, Component component, float value) {
		return {
			(component == Component.RED) ? value : color.red,
			(component == Component.GREEN) ? value : color.green,
			(component == Component.BLUE) ? value : color.blue,
			(component == Component.ALPHA) ? value : color.alpha,
		};
	}

	public static Gdk.RGBA apply_alpha (Gdk.RGBA color, Gdk.RGBA background) {
		var beta = 1 - color.alpha;
		return {
			color.red * color.alpha + background.red * beta,
			color.green * color.alpha + background.green * beta,
			color.blue * color.alpha + background.blue * beta,
			1
		};
	}

	public struct HSV {
		float hue;
		float saturation;
		float value;
		float alpha;

		public HSV.from_rgba (Gdk.RGBA rgba) {
			Color.get_hsv_from_rgba (rgba, out hue, out saturation, out value, out alpha);
		}

		public Gdk.RGBA to_rgba () {
			return Color.get_rgba_from_hsv (hue, saturation, value, alpha);
		}
	}

	public static Gdk.RGBA interpolate (Gdk.RGBA color0, Gdk.RGBA color1, float f) {
		var g = 1 - f;
		var result = Gdk.RGBA () {
			red = color0.red * f + color1.red * g,
			green = color0.green * f + color1.green * g,
			blue = color0.blue * f + color1.blue * g,
			alpha = color0.alpha * f + color1.alpha * g,
		};
		return result;
	}

	public static Gdk.RGBA interpolate_hue (Gdk.RGBA color0, Gdk.RGBA color1, float f) {
		var hsv0 = HSV.from_rgba (color0);
		var hsv1 = HSV.from_rgba (color1);
		var g = 1 - f;
		var result = HSV () {
			hue = hsv0.hue * f + hsv1.hue * g,
			saturation = hsv0.saturation * f + hsv1.saturation * g,
			value = hsv0.value * f + hsv1.value * g,
			alpha = hsv0.alpha * f + hsv1.alpha * g,
		};
		return result.to_rgba ();
	}

	public static Cairo.Pattern build_transparency_pattern (int half_size) {
		var size = half_size * 2;
		var surface = new Cairo.ImageSurface (Cairo.Format.RGB24, size, size);
		var ctx = new Cairo.Context (surface);

		ctx.set_source_rgba (0.66, 0.66, 0.66, 1.0);
		ctx.rectangle (0, 0, half_size, half_size);
		ctx.fill ();
		ctx.rectangle (half_size, half_size, half_size, half_size);
		ctx.fill ();

		ctx.set_source_rgba (0.33, 0.33, 0.33, 1.0);
		ctx.rectangle (half_size, 0, half_size, half_size);
		ctx.fill ();
		ctx.rectangle (0, half_size, half_size, half_size);
		ctx.fill ();

		var transparency_pattern = new Cairo.Pattern.for_surface (surface);
		transparency_pattern.set_extend (Cairo.Extend.REPEAT);
		return transparency_pattern;
	}
}
