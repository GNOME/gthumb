#include <config.h>
#include <math.h>
#include "lib/gth-image.h"
#include "lib/lib.h"

GthImage * gth_image_gaussian_blur (GthImage *source, int radius, GCancellable *cancellable) {
	if (radius <= 0) {
		return gth_image_dup (source);
	}

	int rowstride, width, height;
	guchar *p_src_row = gth_image_prepare_edit (source, &rowstride, &width, &height);

	GthImage *destination = gth_image_new (width, height);
	guchar *p_dest_row = gth_image_prepare_edit (destination, NULL, NULL, NULL);

	int kernel_size = radius * 2 + 1;
	double *weights = g_new (double, kernel_size);
	double sigma = (double) kernel_size / 6.0;
	double k1 = 1.0 / sqrt (2 * M_PI * sigma * sigma);
	double k2 = -1.0 / (2 * sigma * sigma);
	int idx = 0;
	// g_print ("> kernel (radius: %d, sigma: %f): ", radius, sigma);
	for (int i = -radius; i <= radius; i++) {
		weights[idx] = k1 * exp (k2 * (i * i));
		// g_print ("  %f, ", weights[idx]);
		idx++;
	}
	// g_print ("\n");

	guchar *d_pixel;
	guchar red, green, blue, alpha;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	double w_red, w_green, w_blue, w_alpha;
	int max_x = width - 1;
	int max_y = height - 1;
	guchar *pixel;

	// Horizontal blur

	for (int y = 0; y < height; y++) {
		d_pixel = p_dest_row;

		for (int x = 0; x < width; x++) {
			w_red = w_green = w_blue = w_alpha = 0.0;
			int w_idx = 0;
			for (int i = x - radius; i <= x + radius; i++) {
				pixel = p_src_row + (CLAMP (i, 0, max_x) * 4);
				PIXEL_TO_RGBA (pixel, red, green, blue, alpha);
				w_red += weights[w_idx] * red;
				w_green += weights[w_idx] * green;
				w_blue += weights[w_idx] * blue;
				w_alpha += weights[w_idx] * alpha;
				w_idx++;
			}

			red = PIXEL_CLAMP (w_red);
			green = PIXEL_CLAMP (w_green);
			blue = PIXEL_CLAMP (w_blue);
			alpha = PIXEL_CLAMP (w_alpha);
			RGBA_TO_PIXEL (d_pixel, red, green, blue, alpha);

			d_pixel += 4;
		}
		p_src_row += rowstride;
		p_dest_row += rowstride;

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			g_object_unref (destination);
			destination = NULL;
			break;
		}
	}

	if (destination == NULL) {
		g_free (weights);
		return NULL;
	}

	// Vertical blur

	source = destination;
	destination = gth_image_new (width, height);

	p_src_row = gth_image_prepare_edit (source, NULL, NULL, NULL);
	p_dest_row = gth_image_prepare_edit (destination, NULL, NULL, NULL);

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			w_red = w_green = w_blue = w_alpha = 0.0;
			int w_idx = 0;
			for (int j = y - radius; j <= y + radius; j++) {
				pixel = p_src_row + (CLAMP (j, 0, max_y) * rowstride);
				PIXEL_TO_RGBA (pixel, red, green, blue, alpha);
				w_red += weights[w_idx] * red;
				w_green += weights[w_idx] * green;
				w_blue += weights[w_idx] * blue;
				w_alpha += weights[w_idx] * alpha;
				w_idx++;
			}

			d_pixel = p_dest_row + (y * rowstride);
			red = PIXEL_CLAMP (w_red);
			green = PIXEL_CLAMP (w_green);
			blue = PIXEL_CLAMP (w_blue);
			alpha = PIXEL_CLAMP (w_alpha);
			RGBA_TO_PIXEL (d_pixel, red, green, blue, alpha);
		}
		p_src_row += 4;
		p_dest_row += 4;

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			g_object_unref (destination);
			destination = NULL;
			break;
		}
	}

	g_free (weights);
	g_object_unref (source); // first destination

	return destination;
}

gboolean gth_image_progressive_blur (GthImage *source, int max_radius, GCancellable *cancellable) {
	if (max_radius <= 0) {
		return TRUE;
	}

	gboolean completed = TRUE;
	int rowstride, width, height;
	guchar *p_src_row = gth_image_prepare_edit (source, &rowstride, &width, &height);

	GthImage *destination = gth_image_new (width, height);
	guchar *p_dest_row = gth_image_prepare_edit (destination, NULL, NULL, NULL);

	double min_distance = (MIN (width, height) / 2) / 4;
	double max_distance = min_distance + 100;
	double sigma = max_radius * 2 + 1;
	double k1 = 1.0 / sqrt (2 * M_PI * sigma * sigma);
	double k2 = -1.0 / (2 * sigma * sigma);

	guchar *d_pixel;
	guchar red, green, blue, alpha = 255;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	double w_red, w_green, w_blue;
	int max_x = width - 1;
	int max_y = height - 1;
	guchar *pixel;

	// TODO: allow to specify the center
	int center_x = width / 2;
	int center_y = height / 2;
	double dx, dy, distance;
	double radius;
	double weight;

	// Horizontal blur

	for (int y = 0; y < height; y++) {
		d_pixel = p_dest_row;

		for (int x = 0; x < width; x++) {
			dx = x - center_x;
			dy = y - center_y;
			distance = round (sqrt (dx * dx + dy * dy));
			if (distance < min_distance) {
				distance = min_distance;
			}
			distance -= min_distance;
			if (distance > max_distance) {
				distance = max_distance;
			}
			radius = (int) round ((double) max_radius * (distance / max_distance));

			// if (radius <= 1) {
			// 	red = green = blue = 0;
			// 	RGBA_TO_PIXEL (d_pixel, red, green, blue, alpha);
			// 	d_pixel += 4;
			// 	continue;
			// }

			// if (radius >= max_radius) {
			// 	red = green = blue = 255;
			// 	RGBA_TO_PIXEL (d_pixel, red, green, blue, alpha);
			// 	d_pixel += 4;
			// 	continue;
			// }

			if (radius <= 1) {
				pixel = p_src_row + (x * 4);
				PIXEL_TO_RGBA (pixel, red, green, blue, alpha);
				RGBA_TO_PIXEL (d_pixel, red, green, blue, alpha);
				d_pixel += 4;
				continue;
			}

			sigma = (radius * 2 + 1) / 6;
			k1 = 1.0 / sqrt (2 * M_PI * sigma * sigma);
			k2 = -1.0 / (2 * sigma * sigma);

			w_red = w_green = w_blue = 0.0;
			distance = - radius;
			for (int i = x - radius; i <= x + radius; i++) {
				pixel = p_src_row + (CLAMP (i, 0, max_x) * 4);
				PIXEL_TO_RGBA (pixel, red, green, blue, alpha);
				weight = k1 * exp (k2 * (distance * distance));
				w_red += weight * red;
				w_green += weight * green;
				w_blue += weight * blue;
				distance += 1;
			}

			w_red = round (w_red);
			w_green = round (w_green);
			w_blue = round (w_blue);

			red = PIXEL_CLAMP (w_red);
			green = PIXEL_CLAMP (w_green);
			blue = PIXEL_CLAMP (w_blue);
			RGBA_TO_PIXEL (d_pixel, red, green, blue, alpha);

			d_pixel += 4;
		}
		p_src_row += rowstride;
		p_dest_row += rowstride;

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			completed = FALSE;
			break;
		}
	}

	if (!completed) {
		g_object_unref (destination);
		return FALSE;
	}

	// Vertical blur

	p_src_row = gth_image_prepare_edit (destination, NULL, NULL, NULL);
	p_dest_row = gth_image_prepare_edit (source, NULL, NULL, NULL);

	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			dx = x - center_x;
			dy = y - center_y;
			distance = round (sqrt (dx * dx + dy * dy));
			if (distance < min_distance) {
				distance = min_distance;
			}
			distance -= min_distance;
			if (distance > max_distance) {
				distance = max_distance;
			}
			radius = (int) round ((double) max_radius * (distance / max_distance));

			if (radius <= 1) {
				pixel = p_src_row + (y * rowstride);
				PIXEL_TO_RGBA (pixel, red, green, blue, alpha);
				d_pixel = p_dest_row + (y * rowstride);
				RGBA_TO_PIXEL (d_pixel, red, green, blue, alpha);
				continue;
			}

			sigma = (radius * 2 + 1) / 6;
			k1 = 1.0 / sqrt (2 * M_PI * sigma * sigma);
			k2 = -1.0 / (2 * sigma * sigma);

			w_red = w_green = w_blue = 0.0;
			distance = - radius;
			for (int j = y - radius; j <= y + radius; j++) {
				pixel = p_src_row + (CLAMP (j, 0, max_y) * rowstride);
				PIXEL_TO_RGBA (pixel, red, green, blue, alpha);
				weight = k1 * exp (k2 * (distance * distance));
				w_red += weight * red;
				w_green += weight * green;
				w_blue += weight * blue;
				distance += 1;
			}

			w_red = round (w_red);
			w_green = round (w_green);
			w_blue = round (w_blue);

			red = PIXEL_CLAMP (w_red);
			green = PIXEL_CLAMP (w_green);
			blue = PIXEL_CLAMP (w_blue);

			d_pixel = p_dest_row + (y * rowstride);
			RGBA_TO_PIXEL (d_pixel, red, green, blue, alpha);
		}
		p_src_row += 4;
		p_dest_row += 4;

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			completed = FALSE;
			break;
		}
	}

	g_object_unref (destination);
	return completed;
}

static gboolean _box_blur (GthImage *source, GthImage *destination, int radius,
	guchar *div_kernel_size, GCancellable *cancellable)
{
	int src_rowstride, width, height;
	guchar *p_src = gth_image_prepare_edit (source, &src_rowstride, &width, &height);

	int dest_rowstride;
	guchar *p_dest = gth_image_prepare_edit (destination, &dest_rowstride, NULL, NULL);

	int radius_plus_1 = radius + 1;
	int r, g, b, a;
	guchar *c1, *c2;
	int x, y, i, i1, i2;

	// Horizontal blur

	int width_minus_1 = width - 1;
	guchar *p_dest_row;
	for (y = 0; y < height; y++) {
		// Calculate the initial sums of the kernel.

		r = g = b = a = 0;

		for (i = -radius; i <= radius; i++) {
			c1 = p_src + (CLAMP (i, 0, width_minus_1) * 4);
			r += c1[PIXEL_RED];
			g += c1[PIXEL_GREEN];
			b += c1[PIXEL_BLUE];
			a += c1[PIXEL_ALPHA];
		}

		p_dest_row = p_dest;
		for (x = 0; x < width; x++) {
			// Set as the mean of the kernel.

			p_dest_row[PIXEL_RED] = div_kernel_size[r];
			p_dest_row[PIXEL_GREEN] = div_kernel_size[g];
			p_dest_row[PIXEL_BLUE] = div_kernel_size[b];
			p_dest_row[PIXEL_ALPHA] = div_kernel_size[a];
			p_dest_row += 4;

			// The pixel to add to the kernel.

			i1 = x + radius_plus_1;
			if (i1 > width_minus_1) {
				i1 = width_minus_1;
			}
			c1 = p_src + (i1 * 4);

			// The pixel to remove from the kernel.

			i2 = x - radius;
			if (i2 < 0) {
				i2 = 0;
			}
			c2 = p_src + (i2 * 4);

			// Calculate the new sums of the kernel.

			r += c1[PIXEL_RED] - c2[PIXEL_RED];
			g += c1[PIXEL_GREEN] - c2[PIXEL_GREEN];
			b += c1[PIXEL_BLUE] - c2[PIXEL_BLUE];
			a += c1[PIXEL_ALPHA] - c2[PIXEL_ALPHA];
		}

		p_src += src_rowstride;
		p_dest += dest_rowstride;

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			return FALSE;
		}
	}

	// Vertical blur

	p_src = gth_image_prepare_edit (destination, &src_rowstride, NULL, NULL);
	p_dest = gth_image_prepare_edit (source, &dest_rowstride, NULL, NULL);

	int height_minus_1 = height - 1;
	guchar *p_dest_col;
	for (x = 0; x < width; x++) {
		// Calculate the initial sums of the kernel

		r = g = b = a = 0;

		for (i = -radius; i <= radius; i++) {
			c1 = p_src + (CLAMP (i, 0, height_minus_1) * src_rowstride);
			r += c1[PIXEL_RED];
			g += c1[PIXEL_GREEN];
			b += c1[PIXEL_BLUE];
			a += c1[PIXEL_ALPHA];
		}

		p_dest_col = p_dest;
		for (y = 0; y < height; y++) {
			// Set as the mean of the kernel

			p_dest_col[PIXEL_RED] = div_kernel_size[r];
			p_dest_col[PIXEL_GREEN] = div_kernel_size[g];
			p_dest_col[PIXEL_BLUE] = div_kernel_size[b];
			p_dest_col[PIXEL_ALPHA] = div_kernel_size[a];
			p_dest_col += dest_rowstride;

			// The pixel to add to the kernel.

			i1 = y + radius_plus_1;
			if (i1 > height_minus_1) {
				i1 = height_minus_1;
			}
			c1 = p_src + (i1 * src_rowstride);

			// The pixel to remove from the kernel.

			i2 = y - radius;
			if (i2 < 0) {
				i2 = 0;
			}
			c2 = p_src + (i2 * src_rowstride);

			// Calculate the new sums of the kernel.

			r += c1[PIXEL_RED] - c2[PIXEL_RED];
			g += c1[PIXEL_GREEN] - c2[PIXEL_GREEN];
			b += c1[PIXEL_BLUE] - c2[PIXEL_BLUE];
			a += c1[PIXEL_ALPHA] - c2[PIXEL_ALPHA];
		}

		p_src += 4;
		p_dest += 4;

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			return FALSE;
		}
	}

	return TRUE;
}

static GthImage * _box_blur_iterated (GthImage *image, int radius, int iterations, GCancellable *cancellable) {
	// Optimization to avoid divisions: div_kernel_size[x] == x / kernel_size
	gint64 kernel_size = 2 * radius + 1;
	guchar *div_kernel_size = g_new (guchar, 256 * kernel_size);
	for (int i = 0; i < 256 * kernel_size; i++) {
		div_kernel_size[i] = (guchar) (i / kernel_size);
	}

	GthImage *blurred = gth_image_dup (image);
	GthImage *tmp = gth_image_new (gth_image_get_width (image), gth_image_get_height (image));
	gboolean completed = TRUE;
	while (iterations-- > 0) {
		if (!_box_blur (blurred, tmp, radius, div_kernel_size, cancellable)) {
			completed = FALSE;
			break;
		}
	}

	g_object_unref (tmp);
	if (!completed) {
		g_object_unref (blurred);
		blurred = NULL;
	}
	g_free (div_kernel_size);

	return blurred;
}

GthImage * gth_image_blur (GthImage *self, int radius, GCancellable *cancellable) {
	return _box_blur_iterated (self, radius, 3, cancellable);
}

gboolean gth_image_sharpen (GthImage *source, double amount, int radius, double threshold, GCancellable *cancellable) {
	amount = -amount;

	GthImage *blurred = gth_image_blur (source, radius, cancellable);
	if (blurred == NULL) {
		return FALSE;
	}

	int src_rowstride, width, height;
	guchar *p_src = gth_image_prepare_edit (source, &src_rowstride, &width, &height);

	int blurred_rowstride;
	guchar *p_blurred = gth_image_prepare_edit (blurred, &blurred_rowstride, NULL, NULL);

#define ASSIGN_INTERPOLATED_VALUE(x1, x2) \
	if (ABS (x1 - x2) >= threshold) { \
		tmp = INTERPOLATE (x1, x2, amount); \
		x1 = CLAMP (tmp, 0, 255); \
	}

	int x, y;
	guchar *p_src_row, *p_blurred_row;
	guchar r1, g1, b1;
	guchar r2, g2, b2;
	int tmp;
	for (y = 0; y < height; y++) {
		p_src_row = p_src;
		p_blurred_row = p_blurred;
		for (x = 0; x < width; x++) {
			r1 = p_src_row[PIXEL_RED];
			g1 = p_src_row[PIXEL_GREEN];
			b1 = p_src_row[PIXEL_BLUE];

			r2 = p_blurred_row[PIXEL_RED];
			g2 = p_blurred_row[PIXEL_GREEN];
			b2 = p_blurred_row[PIXEL_BLUE];

			ASSIGN_INTERPOLATED_VALUE (r1, r2)
			ASSIGN_INTERPOLATED_VALUE (g1, g2)
			ASSIGN_INTERPOLATED_VALUE (b1, b2)

			p_src_row[PIXEL_RED] = r1;
			p_src_row[PIXEL_GREEN] = g1;
			p_src_row[PIXEL_BLUE] = b1;

			p_src_row += 4;
			p_blurred_row += 4;
		}

		p_src += src_rowstride;
		p_blurred += blurred_rowstride;

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			g_object_unref (blurred);
			return FALSE;
		}
	}

#undef ASSIGN_INTERPOLATED_VALUE

	g_object_unref (blurred);

	return TRUE;
}
