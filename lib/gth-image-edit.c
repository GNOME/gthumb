#include <config.h>
#include <math.h>
#include "lib/gth-curve.h"
#include "lib/gth-histogram.h"
#include "lib/gth-image.h"
#include "lib/gth-point.h"
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

// f is aleady alpha multiplied
#define RENDER_BLEND(b, f, a) \
	temp = ((a) * (b)) + 0x80; \
	b = f + ((temp + (temp >> 8)) >> 8);

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
					guint temp;
					uint8_t *background = (uint8_t*) pixel_p;
					uint8_t *foreground = (uint8_t*) foreground_pixel_p;
					uint8_t alpha_comp = 0xFF - foreground[PIXEL_ALPHA];
					RENDER_BLEND(background[PIXEL_RED], foreground[PIXEL_RED], alpha_comp);
					RENDER_BLEND(background[PIXEL_GREEN], foreground[PIXEL_GREEN], alpha_comp);
					RENDER_BLEND(background[PIXEL_BLUE], foreground[PIXEL_BLUE], alpha_comp);
					RENDER_BLEND(background[PIXEL_ALPHA], foreground[PIXEL_ALPHA], alpha_comp);
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

#undef RENDER_BLEND

gboolean gth_image_grayscale (GthImage *self, double red_weight, double green_weight, double blue_weight, double amount, GCancellable *cancellable) {
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
		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			return FALSE;
		}
	}
	return TRUE;
}

gboolean gth_image_grayscale_saturation (GthImage *self, double amount, GCancellable *cancellable) {
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
		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			return FALSE;
		}
	}
	return TRUE;
}

gboolean gth_image_gamma_correction (GthImage *self, double gamma, GCancellable *cancellable) {
	if (gamma == 1.0) {
		return TRUE;
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
		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			return FALSE;
		}
	}
	return TRUE;
}

gboolean gth_image_adjust_brightness (GthImage *self, double amount, GCancellable *cancellable) {
	if (amount == 0) {
		return TRUE;
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
		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			return FALSE;
		}
	}
	return TRUE;
}

gboolean gth_image_adjust_contrast (GthImage *self, double amount, GCancellable *cancellable) {
	if (amount == 0) {
		return TRUE;
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
		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			return FALSE;
		}
	}
	return TRUE;
}

void calc_radial_mask (guint width, guint height, double amount, GthPoint *f1, GthPoint *f2, double *min_d, double *max_d) {
	// Do not apply the effect inside the ellipse with a,b diameters
	// at the center of the image.
	// https://en.wikipedia.org/wiki/Ellipse

	double center_x = width / 2.0;
	double center_y = height / 2.0;
	double b = MIN (height, width) / 6.0;
	double a = b; //MAX (width, height) / 6.0;

	double eccentricity = sqrt (1.0 - SQR (b) / SQR (a));
	// c = distance from the center to a focus
	double c = a * eccentricity;

	// Do not apply the effect for points with a distance less
	// than min_d
	*min_d = 2 * sqrt (SQR (b) + SQR (c));

	// Focus points.
	if (width > height) {
		f1->x = center_x - c;
		f1->y = center_y;
		f2->x = center_x + c;
		f2->y = center_y;
	}
	else {
		f1->x = center_x;
		f1->y = center_y - c;
		f2->x = center_x;
		f2->y = center_y + c;
	}

	// Apply the effect without transparency for points with a
	// distance greater than max_d.
	GthPoint p = { .x = 0, .y = 0 };
	// p.x = 0;
	// p.y = 0;
	*max_d = gth_point_distance (&p, f1) + gth_point_distance (&p, f2);

	// The greater the `amount` the larger the image area where the
	// effect is applied.
	// Reduce the min and max distance by 10% when `amount` is 1.
	// amount: 0 -> distance: 100%
	// amount: 1 -> distance: 95%
	*min_d *= (1.0 - (amount * (1 - 0.95)));
	*max_d *= (1.0 - (amount * (1 - 0.95)));
}

gboolean gth_image_apply_vignette (GthImage *self, double amount, GCancellable *cancellable) {
	// The greater `amount` the darker the edges of the image.
	GthPoint start_point_ligher = { 0, 0 };
	GthPoint start_point_darker = { 190, 0 };
	GthPoint start_point;
	gth_point_init_interpolate (&start_point, &start_point_ligher, &start_point_darker, amount);

	GthPoint end_point_ligher = { 255, 255 };
	GthPoint end_point_darker = { 255, 65 };
	GthPoint end_point;
	gth_point_init_interpolate (&end_point, &end_point_ligher, &end_point_darker, amount);
	GthPoint value_points[] = { start_point, end_point };

	GthPoints points;
	gth_points_init (&points, value_points, 2, NULL, 0, NULL, 0, NULL, 0);
	guchar *value_map = gth_points_get_value_map (&points, NULL);

	const guchar *red_map = value_map + (GTH_CHANNEL_RED * 256);
	const guchar *green_map = value_map + (GTH_CHANNEL_GREEN * 256);
	const guchar *blue_map = value_map + (GTH_CHANNEL_BLUE * 256);

	if (amount < 0) {
		amount = -amount;
	}

	int row_stride;
	guint width;
	guint height;
	guchar *row = gth_image_prepare_edit (self, &row_stride, &width, &height);
	guchar *pixel;
	guchar red, green, blue, alpha;
	guchar new_red, new_green, new_blue;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	double new_alpha;
	guint temp; // used in RGBA_TO_PIXEL
	GthPoint f1, f2;
	double min_d;
	double max_d;
	calc_radial_mask (width, height, amount, &f1, &f2, &min_d, &max_d);

	gboolean cancelled = FALSE;
	for (guint y = 0; y < height; y++) {
		pixel = row;
		for (guint x = 0; x < width; x++) {
			GthPoint p;
			p.x = x;
			p.y = y;
			double d = gth_point_distance (&p, &f1) + gth_point_distance (&p, &f2);
			if (d >= min_d) {
				PIXEL_TO_RGBA (pixel, red, green, blue, alpha);
				new_red = red_map[red];
				new_green = green_map[green];
				new_blue = blue_map[blue];

				// d <= min_d -> alpha = 0 -> show original image
				// d >= max_d -> alpha = 1 -> show new image
				if (d >= max_d) {
					new_alpha = 1.0;
				}
				else {
					new_alpha = (d - min_d) / (max_d - min_d);
				}

				red = PIXEL_OVER (red, new_red, new_alpha);
				green = PIXEL_OVER (green, new_green, new_alpha);
				blue = PIXEL_OVER (blue, new_blue, new_alpha);
				RGBA_TO_PIXEL (pixel, red, green, blue, alpha);
			}
			// if (d >= max_d) {
			// 	pixel[PIXEL_RED] = 255;
			// 	pixel[PIXEL_GREEN] = 0;
			// 	pixel[PIXEL_BLUE] = 0;
			// }
			// else if (d <= min_d) {
			// 	pixel[PIXEL_RED] = 0;
			// 	pixel[PIXEL_GREEN] = 255;
			// 	pixel[PIXEL_BLUE] = 0;
			// }
			pixel += 4;
		}
		row += row_stride;
		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			cancelled = TRUE;
			break;
		}
	}

	g_free (value_map);

	return !cancelled;
}

gboolean gth_image_apply_radial_mask (GthImage *background, GthImage *foreground, double amount, GCancellable *cancellable) {
	int b_row_stride;
	guint b_width;
	guint b_height;
	guchar *b_row = gth_image_prepare_edit (background, &b_row_stride, &b_width, &b_height);
	guchar *b_pixel;

	int f_row_stride;
	guint f_width;
	guint f_height;
	guchar *f_row = gth_image_prepare_edit (foreground, &f_row_stride, &f_width, &f_height);
	guchar *f_pixel;

	g_return_val_if_fail ((b_width == f_width) && (b_height == f_height), FALSE);

	guchar b_red, b_green, b_blue, b_alpha;
	guchar f_red, f_green, f_blue, f_alpha;
	guint temp; // used in RGBA_TO_PIXEL
	guchar r, g, b; // used in RGBA_TO_PIXEL
	GthPoint f1, f2;
	double min_d, max_d, alpha;
	calc_radial_mask (b_width, b_height, amount, &f1, &f2, &min_d, &max_d);

	gboolean cancelled = FALSE;
	for (guint y = 0; y < b_height; y++) {
		b_pixel = b_row;
		f_pixel = f_row;
		for (guint x = 0; x < b_width; x++) {
			GthPoint p;
			p.x = x;
			p.y = y;
			double d = gth_point_distance (&p, &f1) + gth_point_distance (&p, &f2);
			if (d >= min_d) {
				PIXEL_TO_RGBA (b_pixel, b_red, b_green, b_blue, b_alpha);
				PIXEL_TO_RGBA (f_pixel, f_red, f_green, f_blue, f_alpha);

				// d <= min_d -> alpha = 0 -> show background
				// d >= max_d -> alpha = 1 -> show foreground
				if (d >= max_d) {
					alpha = 1.0;
				}
				else {
					alpha = (d - min_d) / (max_d - min_d);
				}

				b_red = PIXEL_OVER (b_red, f_red, alpha);
				b_green = PIXEL_OVER (b_green, f_green, alpha);
				b_blue = PIXEL_OVER (b_blue, f_blue, alpha);
				RGBA_TO_PIXEL (b_pixel, b_red, b_green, b_blue, b_alpha);
			}
			// if (d >= max_d) {
			// 	b_pixel[PIXEL_RED] = 255;
			// 	b_pixel[PIXEL_GREEN] = 0;
			// 	b_pixel[PIXEL_BLUE] = 0;
			// }
			// else if (d <= min_d) {
			// 	b_pixel[PIXEL_RED] = 0;
			// 	b_pixel[PIXEL_GREEN] = 255;
			// 	b_pixel[PIXEL_BLUE] = 0;
			// }
			b_pixel += 4;
			f_pixel += 4;
		}
		b_row += b_row_stride;
		f_row += f_row_stride;
		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			cancelled = TRUE;
			break;
		}
	}

	return !cancelled;
}

gboolean gth_image_apply_value_map (GthImage *self, guchar *value_map, GCancellable *cancellable) {
	int row_stride;
	guint width;
	guint height;
	guchar *row = gth_image_prepare_edit (self, &row_stride, &width, &height);
	guchar *pixel;
	guchar red, green, blue, alpha;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	guchar *red_map = value_map + (GTH_CHANNEL_RED * VALUE_MAP_COLUMNS);
	guchar *green_map = value_map + (GTH_CHANNEL_GREEN * VALUE_MAP_COLUMNS);
	guchar *blue_map = value_map + (GTH_CHANNEL_BLUE * VALUE_MAP_COLUMNS);
	for (guint y = 0; y < height; y++) {
		pixel = row;
		for (guint x = 0; x < width; x++) {
			PIXEL_TO_RGBA (pixel, red, green, blue, alpha);
			red = red_map[red];
			green = green_map[green];
			blue = blue_map[blue];
			RGBA_TO_PIXEL (pixel, red, green, blue, alpha);
			pixel += 4;
		}
		row += row_stride;
		if ((cancellable != NULL) && (g_cancellable_is_cancelled (cancellable))) {
			return FALSE;
		}
	}
	return TRUE;
}

gboolean gth_image_apply_curve (GthImage *self, GthPoints *points, GCancellable *cancellable)
{
	if (points == NULL) {
		return TRUE;
	}
	guchar *value_map = gth_points_get_value_map (points, NULL);
	gboolean completed = gth_image_apply_value_map (self, value_map, cancellable);
	g_free (value_map);
	return completed;
}

gboolean gth_image_colorize (GthImage *self, double red_amount, double green_amount, double blue_amount, GCancellable *cancellable) {
	GthPoint lighter_point = { 127, 127 };
	GthPoint darker_point = { 82, 187 };
	GthPoint middle_point;

	gth_point_init_interpolate (&middle_point, &lighter_point, &darker_point, red_amount);
	GthPoint red_points[] = { { 0, 0 }, middle_point, { 255, 255 } };

	gth_point_init_interpolate (&middle_point, &lighter_point, &darker_point, green_amount);
	GthPoint green_points[] = { { 0, 0 }, middle_point, { 255, 255 } };

	gth_point_init_interpolate (&middle_point, &lighter_point, &darker_point, blue_amount);
	GthPoint blue_points[] = { { 0, 0 }, middle_point, { 255, 255 } };

	GthPoints points;
	gth_points_init (&points, NULL, 0,
		red_points, (red_amount > 0) ? 3 : 0,
		green_points, (green_amount > 0) ? 3 : 0,
		blue_points, (blue_amount > 0) ? 3 : 0);
	return gth_image_apply_curve (self, &points, cancellable);
}

gboolean gth_image_soft_light_with_radial_gradient (GthImage *self, GCancellable *cancellable) {
	int row_stride;
	guint width;
	guint height;
	guchar *row = gth_image_prepare_edit (self, &row_stride, &width, &height);
	guchar *pixel;
	guchar red, green, blue, alpha;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL

	double center_x = width / 2.0;
	double center_y = height / 2.0;
	double radius = MAX (width, height) / 2.0;

	gboolean cancelled = FALSE;
	for (guint y = 0; y < height; y++) {
		pixel = row;
		for (guint x = 0; x < width; x++) {
			PIXEL_TO_RGBA (pixel, red, green, blue, alpha);

			double distance = sqrt (SQR (x - center_x) + SQR (y - center_y));
			guchar gradient_value = PIXEL_CLAMP ((distance >= radius) ? 0 : 255 - (255.0 * (distance / radius)));

			red = PIXEL_SOFT_LIGHT (red, gradient_value);
			green = PIXEL_SOFT_LIGHT (green, gradient_value);
			blue = PIXEL_SOFT_LIGHT (blue, gradient_value);
			RGBA_TO_PIXEL (pixel, red, green, blue, alpha);

			pixel += 4;
		}
		row += row_stride;
		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			cancelled = TRUE;
			break;
		}
	}
	return !cancelled;
}
