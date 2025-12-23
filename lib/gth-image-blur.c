#include <config.h>
#include <math.h>
#include "lib/gth-image.h"
#include "lib/lib.h"

static gboolean _gaussian_blur (GthImage *source, int radius, GCancellable *cancellable) {
	// TODO
	return FALSE;
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
		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable))
			return FALSE;

		// Calculate the initial sums of the kernel.

		r = g = b = a = 0;

		for (i = -radius; i <= radius; i++) {
			c1 = p_src + (CLAMP (i, 0, width_minus_1) * 4);
			r += c1[PIXEL_RED];
			g += c1[PIXEL_GREEN];
			b += c1[PIXEL_BLUE];
			// if (n_channels == 4) {
			// 	a += c1[PIXEL_ALPHA];
			// }
		}

		p_dest_row = p_dest;
		for (x = 0; x < width; x++) {
			// Set as the mean of the kernel.

			p_dest_row[PIXEL_RED] = div_kernel_size[r];
			p_dest_row[PIXEL_GREEN] = div_kernel_size[g];
			p_dest_row[PIXEL_BLUE] = div_kernel_size[b];
			p_dest_row[PIXEL_ALPHA] = 0xff;
			// if (n_channels == 4) {
			// 	p_dest_row[PIXEL_ALPHA] = div_kernel_size[a];
			// }
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
			// if (n_channels == 4) {
			// 	a += c1[PIXEL_ALPHA] - c2[PIXEL_ALPHA];
			// }
		}

		p_src += src_rowstride;
		p_dest += dest_rowstride;
	}

	// Vertical blur

	p_src = gth_image_prepare_edit (destination, &dest_rowstride, NULL, NULL);
	p_dest = gth_image_prepare_edit (source, &src_rowstride, NULL, NULL);

	int height_minus_1 = height - 1;
	guchar *p_dest_col;
	for (x = 0; x < width; x++) {
		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable))
			return FALSE;

		// Calculate the initial sums of the kernel

		r = g = b = a = 0;

		for (i = -radius; i <= radius; i++) {
			c1 = p_src + (CLAMP (i, 0, height_minus_1) * src_rowstride);
			r += c1[PIXEL_RED];
			g += c1[PIXEL_GREEN];
			b += c1[PIXEL_BLUE];
			// if (n_channels == 4) {
			// 	a += c1[PIXEL_ALPHA];
			// }
		}

		p_dest_col = p_dest;
		for (y = 0; y < height; y++) {
			// Set as the mean of the kernel

			p_dest_col[PIXEL_RED] = div_kernel_size[r];
			p_dest_col[PIXEL_GREEN] = div_kernel_size[g];
			p_dest_col[PIXEL_BLUE] = div_kernel_size[b];
			p_dest_col[PIXEL_ALPHA] = 0xff;
			// if (n_channels == 4) {
			// 	p_dest_row[PIXEL_ALPHA] = div_kernel_size[a];
			// }
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
			// if (n_channels == 4) {
			// 	a += c1[PIXEL_ALPHA] - c2[PIXEL_ALPHA];
			// }
		}

		p_src += 4;
		p_dest += 4;
	}

	return TRUE;
}


static gboolean _box_blur_iterated (GthImage *image, int radius, int iterations, GCancellable *cancellable) {
	// Optimization to avoid divisions: div_kernel_size[x] == x / kernel_size
	gint64 kernel_size = 2 * radius + 1;
	guchar *div_kernel_size = g_new (guchar, 256 * kernel_size);
	for (int i = 0; i < 256 * kernel_size; i++) {
		div_kernel_size[i] = (guchar) (i / kernel_size);
	}

	gboolean completed = TRUE;
	GthImage *tmp = gth_image_new (gth_image_get_width (image), gth_image_get_height (image));
	while (completed && (iterations-- > 0)) {
		completed = _box_blur (image, tmp, radius, div_kernel_size, cancellable);
	}
	g_object_unref (tmp);
	g_free (div_kernel_size);

	return completed;
}

gboolean gth_image_blur (GthImage *self, int radius, GCancellable *cancellable) {
	if (radius <= 10) {
		return _box_blur_iterated (self, radius, 3, cancellable);
	}
	else {
		return _gaussian_blur (self, radius, cancellable);
	}
}

gboolean gth_image_sharpen (GthImage *source, double amount, int radius, double threshold, GCancellable *cancellable) {
	//return gth_image_blur (source, radius, cancellable);

	amount = -amount;

	GthImage *blurred = gth_image_dup (source);
	if (!gth_image_blur (blurred, radius, cancellable)) {
		g_object_unref (blurred);
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

		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			g_object_unref (blurred);
			return FALSE;
		}

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
	}

#undef ASSIGN_INTERPOLATED_VALUE

	g_object_unref (blurred);

	return TRUE;
}
