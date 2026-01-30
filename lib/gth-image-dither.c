#include <config.h>
#include <math.h>
#include "lib/gth-image.h"
#include "lib/lib.h"

static uint8_t THRESHOLD_MAP_2[4] = {
	0, 2,
	3, 1,
};

static uint8_t THRESHOLD_MAP_4[16] = {
	0, 8, 2, 10,
	12, 4, 14, 6,
	3, 11, 1, 9,
	15, 7, 13, 5,
};

static uint8_t THRESHOLD_MAP_8[64] = {
	0, 32, 8, 40, 2, 34, 10, 42,
	48, 16, 56, 24, 50, 18, 58, 26,
	12, 44, 4, 36, 14, 46, 6, 38,
	60, 28, 52, 20, 62, 30, 54, 22,
	3, 35, 11, 43, 1, 33, 9, 41,
	51, 19, 59, 27, 49, 17, 57, 25,
	15, 47, 7, 39, 13, 45, 5, 37,
	63, 31, 55, 23, 61, 29, 53, 21
};

gboolean gth_image_dither_ordered (GthImage *self, GCancellable *cancellable) {
	int row_stride;
	int width;
	int height;
	guchar *row = gth_image_prepare_edit (self, &row_stride, &width, &height);
	guchar *pixel;
	guchar red, green, blue, alpha;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	int map_row;

	int map_size = 8;
	uint8_t *threshold_map = (map_size == 8) ? THRESHOLD_MAP_8 : (map_size == 4) ? THRESHOLD_MAP_4 : THRESHOLD_MAP_2;
	int map_patterns = map_size * map_size;
	int color_reduction = 256 / map_patterns;

	gboolean cancelled = FALSE;
	for (int y = 0; y < height; y++) {
		pixel = row;
		map_row = (y % map_size) * map_size;
		for (int x = 0; x < width; x++) {
			PIXEL_TO_RGBA (pixel, red, green, blue, alpha);

			uint8_t threshold = threshold_map[map_row + (x % map_size)];
			red = (red / color_reduction > threshold) ? 255 : 0;
			green = (green / color_reduction > threshold) ? 255 : 0;
			blue = (blue / color_reduction > threshold) ? 255 : 0;

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

// Jarvis, Judice, and Ninke weights
#define UPDATE_ERRORS(component, error_row_0, error_row_1, error_row_2) \
	value = component; \
	value += error_row_0[x]; \
	new_value = (value < 127) ? 0 : 255; \
	error = value - new_value; \
	if (x > 1) { \
		error_row_1[x - 2] += error / 48 * 3; \
		error_row_2[x - 2] += error / 48 * 1; \
	} \
	if (x > 0) { \
		error_row_1[x - 1] += error / 48 * 5; \
		error_row_2[x - 1] += error / 48 * 3; \
	} \
	error_row_1[x] += error / 48 * 7; \
	error_row_2[x] += error / 48 * 5; \
	if (x < width - 1) { \
		error_row_0[x + 1] += error / 48 * 7; \
		error_row_1[x + 1] += error / 48 * 5; \
		error_row_2[x + 1] += error / 48 * 3; \
	} \
	if (x < width - 2) { \
		error_row_0[x + 2] += error / 48 * 5; \
		error_row_1[x + 2] += error / 48 * 3; \
		error_row_2[x + 2] += error / 48 * 1; \
	}

gboolean gth_image_dither_error_diffusion (GthImage *self, GCancellable *cancellable) {
	int row_stride;
	int width;
	int height;
	guchar *row = gth_image_prepare_edit (self, &row_stride, &width, &height);
	guchar *pixel;
	guchar red, green, blue, alpha;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	double value;
	guchar new_value;

	double *red_error_row_0 = g_new (double, width);
	double *red_error_row_1 = g_new (double, width);
	double *red_error_row_2 = g_new (double, width);
	double *green_error_row_0 = g_new (double, width);
	double *green_error_row_1 = g_new (double, width);
	double *green_error_row_2 = g_new (double, width);
	double *blue_error_row_0 = g_new (double, width);
	double *blue_error_row_1 = g_new (double, width);
	double *blue_error_row_2 = g_new (double, width);
	double error;

	for (int e = 0; e < width; e++) {
		red_error_row_0[e] = 0;
		red_error_row_1[e] = 0;
		red_error_row_2[e] = 0;
		green_error_row_0[e] = 0;
		green_error_row_1[e] = 0;
		green_error_row_2[e] = 0;
		blue_error_row_0[e] = 0;
		blue_error_row_1[e] = 0;
		blue_error_row_2[e] = 0;
	}

	gboolean cancelled = FALSE;
	for (int y = 0; y < height; y++) {
		pixel = row;
		for (int x = 0; x < width; x++) {
			PIXEL_TO_RGBA (pixel, red, green, blue, alpha);

			UPDATE_ERRORS (red, red_error_row_0, red_error_row_1, red_error_row_2);
			red = new_value;

			UPDATE_ERRORS (green, green_error_row_0, green_error_row_1, green_error_row_2);
			green = new_value;

			UPDATE_ERRORS (blue, blue_error_row_0, blue_error_row_1, blue_error_row_2);
			blue = new_value;

			RGBA_TO_PIXEL (pixel, red, green, blue, alpha);
			pixel += 4;
		}
		row += row_stride;
		for (int e = 0; e < width; e++) {
			red_error_row_0[e] = red_error_row_1[e];
			red_error_row_1[e] = red_error_row_2[e];
			red_error_row_2[e] = 0;
			green_error_row_0[e] = green_error_row_1[e];
			green_error_row_1[e] = green_error_row_2[e];
			green_error_row_2[e] = 0;
			blue_error_row_0[e] = blue_error_row_1[e];
			blue_error_row_1[e] = blue_error_row_2[e];
			blue_error_row_2[e] = 0;
		}
		if ((cancellable != NULL) && g_cancellable_is_cancelled (cancellable)) {
			cancelled = TRUE;
			break;
		}
	}

	g_free (red_error_row_0);
	g_free (red_error_row_1);
	g_free (red_error_row_2);
	g_free (green_error_row_0);
	g_free (green_error_row_1);
	g_free (green_error_row_2);
	g_free (blue_error_row_0);
	g_free (blue_error_row_1);
	g_free (blue_error_row_2);

	return !cancelled;
}

#undef UPDATE_ERRORS
