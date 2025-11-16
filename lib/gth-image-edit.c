#include <config.h>
#include <math.h>
#include "lib/gth-image.h"
#include "lib/types.h"
#include "lib/util.h"

void gth_image_fill_vertical (GthImage *self, GthImage *pattern, GthFill fill) {
	int row_stride;
	guint width;
	guint height;
	guchar *pixels = gth_image_prepare_edit (self, &row_stride, &width, &height);

	int pattern_stride;
	guint pattern_width;
	guint pattern_height;
	guchar *pattern_pixels = gth_image_prepare_edit (pattern, &pattern_stride, &pattern_width, &pattern_height);

	if (width <= pattern_width * 2)
		return;

	int first_column = (fill == GTH_FILL_END) ? (width - pattern_width) : 0;
	guchar *pattern_row = pattern_pixels;
	guchar *dest_row = pixels;
	int pattern_row_idx = 0;
	int temp;
	int alpha;
	float factor;

	for (int row_idx = 0; row_idx < height; row_idx++) {
		guchar *dest_pixel = dest_row + (first_column * PIXEL_BYTES);
		guchar *pattern_pixel = pattern_row;
		for (int col = 0; col < pattern_width; col++) {
			alpha = pattern_pixel[PIXEL_ALPHA];
			if (alpha == 0xFF) {
				memcpy (dest_pixel, pattern_pixel, PIXEL_BYTES);
			}
			else {
				factor = (float) (0xFF - alpha) / 0xFF;
				dest_pixel[PIXEL_RED] = PIXEL_CLAMP (factor * dest_pixel[PIXEL_RED] + pattern_pixel[PIXEL_RED]);
				dest_pixel[PIXEL_GREEN] = PIXEL_CLAMP (factor * dest_pixel[PIXEL_GREEN] + pattern_pixel[PIXEL_GREEN]);
				dest_pixel[PIXEL_BLUE] = PIXEL_CLAMP (factor * dest_pixel[PIXEL_BLUE] + pattern_pixel[PIXEL_BLUE]);
			}
			dest_pixel += PIXEL_BYTES;
			pattern_pixel += PIXEL_BYTES;
		}
		dest_row += row_stride;

		// Restart from row 0 if the pattern ends.
		pattern_row_idx += 1;
		if (pattern_row_idx >= pattern_height) {
			pattern_row_idx = 0;
			pattern_row = pattern_pixels;
		}
		else {
			pattern_row += pattern_stride;
		}
	}
}

void gth_image_render_frame (GthImage *canvas, GthImage *background,
	guint32 background_color, GthImage *foreground, guint foreground_x,
	guint foreground_y, gboolean blend)
{
	g_return_if_fail (canvas != NULL);
	g_return_if_fail (foreground != NULL);

	guint canvas_width = gth_image_get_width (canvas);
	guint canvas_height = gth_image_get_height (canvas);
	int image_row_stride;
	guchar *image_row = gth_image_prepare_edit (canvas, &image_row_stride, NULL, NULL);
	guint32 *pixel_p;

	int background_row_stride;
	guchar *background_row = NULL;
	guint32 *background_pixel_p;
	gboolean has_background = background != NULL;
	if (has_background) {
		background_row = gth_image_prepare_edit (background, &background_row_stride, NULL, NULL);
		g_return_if_fail (gth_image_get_width (background) == canvas_width);
		g_return_if_fail (gth_image_get_height (background) == canvas_height);
	}

	int foreground_row_stride;
	guchar *foreground_row = gth_image_prepare_edit (foreground, &foreground_row_stride, NULL, NULL);
	guint foreground_width = gth_image_get_width (foreground);
	guint foreground_height = gth_image_get_height (foreground);
	guint32 *foreground_pixel_p;
	g_return_if_fail (foreground_x + foreground_width <= canvas_width);
	g_return_if_fail (foreground_y + foreground_height <= canvas_height);

	for (int i = 0; i < canvas_height; i++) {
		pixel_p = (guint32 *) image_row;
		background_pixel_p = (guint32 *) background_row;
		foreground_pixel_p = (guint32 *) foreground_row;
		for (int j = 0; j < canvas_width; j++) {
			if (has_background) {
				*pixel_p = *background_pixel_p;
				background_pixel_p += 1;
			}
			else {
				*pixel_p = background_color;
			}
			// Inside the foreground image
			if ((i >= foreground_y)
				&& (i < foreground_y + foreground_height)
				&& (j >= foreground_x)
				&& (j < foreground_x + foreground_width))
			{
				if (blend) {
					pixel_over ((uint8_t*) pixel_p, (uint8_t*) foreground_pixel_p);
				}
				else {
					*pixel_p = *foreground_pixel_p;
				}
				foreground_pixel_p += 1;
			}
			pixel_p += 1;
		}
		image_row += image_row_stride;
		if (has_background) {
			background_row += background_row_stride;
		}
		if (i >= foreground_y) {
			foreground_row += foreground_row_stride;
		}
	}
}

void gth_image_grayscale (GthImage *self, double red_weight, double green_weight, double blue_weight, double amount) {
	if (amount < 0) {
		amount = tan (amount) * G_PI;
	}
	// g_print ("> amount: %f\n", amount);
	int row_stride;
	guint width;
	guint height;
	guchar *row = gth_image_prepare_edit (self, &row_stride, &width, &height);
	guchar *pixel;
	guchar red, green, blue, alpha;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	int value, itemp;
	for (guint y = 0; y < height; y++) {
		pixel = row;
		for (guint x = 0; x < width; x++) {
			PIXEL_TO_RGBA (pixel, red, green, blue, alpha);
			value = (red_weight * red) + (green_weight * green) + (blue_weight * blue);

			itemp = INTERPOLATE (red, value, amount);
			red = CLAMP (itemp, 0, 255);

			itemp = INTERPOLATE (green, value, amount);
			green = CLAMP (itemp, 0, 255);

			itemp = INTERPOLATE (blue, value, amount);
			blue = CLAMP (itemp, 0, 255);

			RGBA_TO_PIXEL (pixel, red, green, blue, alpha);
			pixel += 4;
		}
		row += row_stride;
	}
}

void gth_image_grayscale_saturation (GthImage *self, double amount) {
	if (amount < 0) {
		amount = tan (amount) * G_PI;
	}
	// g_print ("> amount: %f\n", amount);
	int row_stride;
	guint width;
	guint height;
	guchar *row = gth_image_prepare_edit (self, &row_stride, &width, &height);
	guchar *pixel;
	guchar red, green, blue, alpha, min, max;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	int value, itemp;
	for (guint y = 0; y < height; y++) {
		pixel = row;
		for (guint x = 0; x < width; x++) {
			PIXEL_TO_RGBA (pixel, red, green, blue, alpha);
			max = MAX (MAX (red, green), blue);
			min = MIN (MIN (red, green), blue);
			value = (max + min) / 2;

			itemp = INTERPOLATE (red, value, amount);
			red = CLAMP (itemp, 0, 255);

			itemp = INTERPOLATE (green, value, amount);
			green = CLAMP (itemp, 0, 255);

			itemp = INTERPOLATE (blue, value, amount);
			blue = CLAMP (itemp, 0, 255);

			RGBA_TO_PIXEL (pixel, red, green, blue, alpha);
			pixel += 4;
		}
		row += row_stride;
	}
}

void gth_image_gamma_correction (GthImage *self, double gamma) {
	if (gamma == 1.0) {
		return;
	}
	// g_print ("> gamma: %f\n", gamma);
	int row_stride;
	guint width;
	guint height;
	guchar *row = gth_image_prepare_edit (self, &row_stride, &width, &height);
	guchar *pixel;
	guchar red, green, blue, alpha;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	double value;
	for (guint y = 0; y < height; y++) {
		pixel = row;
		for (guint x = 0; x < width; x++) {
			PIXEL_TO_RGBA (pixel, red, green, blue, alpha);

			value = 255.0 * pow (((double) red / 255.0), gamma);
			red = CLAMP (value, 0, 255);

			value = 255.0 * pow (((double) green / 255.0), gamma);
			green = CLAMP (value, 0, 255);

			value = 255.0 * pow (((double) blue / 255.0), gamma);
			blue = CLAMP (value, 0, 255);

			RGBA_TO_PIXEL (pixel, red, green, blue, alpha);
			pixel += 4;
		}
		row += row_stride;
	}
}

void gth_image_adjust_brightness (GthImage *self, double amount) {
	if (amount == 0) {
		return;
	}
	// g_print ("> amount: %f\n", amount);
	int row_stride;
	guint width;
	guint height;
	guchar *row = gth_image_prepare_edit (self, &row_stride, &width, &height);
	guchar *pixel;
	guchar red, green, blue, alpha;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	double dtemp;
	for (guint y = 0; y < height; y++) {
		pixel = row;
		for (guint x = 0; x < width; x++) {
			PIXEL_TO_RGBA (pixel, red, green, blue, alpha);

			if (amount > 0) {
				dtemp = INTERPOLATE (red, 0.0, amount);
				red = CLAMP (dtemp, 0, 255);

				dtemp = INTERPOLATE (green, 0.0, amount);
				green = CLAMP (dtemp, 0, 255);

				dtemp = INTERPOLATE (blue, 0.0, amount);
				blue = CLAMP (dtemp, 0, 255);
			}
			else {
				dtemp = INTERPOLATE (red, 255.0, -amount);
				red = CLAMP (dtemp, 0, 255);

				dtemp = INTERPOLATE (green, 255.0, -amount);
				green = CLAMP (dtemp, 0, 255);

				dtemp = INTERPOLATE (blue, 255.0, -amount);
				blue = CLAMP (dtemp, 0, 255);
			}

			RGBA_TO_PIXEL (pixel, red, green, blue, alpha);
			pixel += 4;
		}
		row += row_stride;
	}
}

void gth_image_adjust_contrast (GthImage *self, double amount) {
	if (amount == 0) {
		return;
	}
	if (amount < 0) {
		amount = tan (amount * G_PI_2);
	}
	// g_print ("> amount: %f\n", amount);
	int row_stride;
	guint width;
	guint height;
	guchar *row = gth_image_prepare_edit (self, &row_stride, &width, &height);
	guchar *pixel;
	guchar red, green, blue, alpha;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	double dtemp;
	for (guint y = 0; y < height; y++) {
		pixel = row;
		for (guint x = 0; x < width; x++) {
			PIXEL_TO_RGBA (pixel, red, green, blue, alpha);

			dtemp = INTERPOLATE (red, 127.0, amount);
			red = CLAMP (dtemp, 0, 255);

			dtemp = INTERPOLATE (green, 127.0, amount);
			green = CLAMP (dtemp, 0, 255);

			dtemp = INTERPOLATE (blue, 127.0, amount);
			blue = CLAMP (dtemp, 0, 255);

			RGBA_TO_PIXEL (pixel, red, green, blue, alpha);
			pixel += 4;
		}
		row += row_stride;
	}
}
