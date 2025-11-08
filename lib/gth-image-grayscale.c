#include <config.h>
#include <math.h>
#include "lib/gth-image.h"
#include "lib/types.h"

void gth_image_grayscale (GthImage *self, double red_weight, double green_weight, double blue_weight) {
	int row_stride;
	guint width;
	guint height;
	guchar *row = gth_image_prepare_edit (self, &row_stride, &width, &height);
	guchar *pixel;
	guchar red, green, blue, alpha, value;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	for (guint y = 0; y < height; y++) {
		pixel = row;
		for (guint x = 0; x < width; x++) {
			PIXEL_TO_RGBA (pixel, red, green, blue, alpha);
			value = (red_weight * red) + (green_weight * green) + (blue_weight * blue);
			RGBA_TO_PIXEL (pixel, value, value, value, alpha);
			pixel += 4;
		}
		row += row_stride;
	}
}

void gth_image_grayscale_saturation (GthImage *self) {
	int row_stride;
	guint width;
	guint height;
	guchar *row = gth_image_prepare_edit (self, &row_stride, &width, &height);
	guchar *pixel;
	guchar red, green, blue, alpha, value, min, max;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	for (guint y = 0; y < height; y++) {
		pixel = row;
		for (guint x = 0; x < width; x++) {
			PIXEL_TO_RGBA (pixel, red, green, blue, alpha);
			max = MAX (MAX (red, green), blue);
			min = MIN (MIN (red, green), blue);
			value = (max + min) / 2;
			RGBA_TO_PIXEL (pixel, value, value, value, alpha);
			pixel += 4;
		}
		row += row_stride;
	}
}
