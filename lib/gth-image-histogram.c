#include <config.h>
#include <math.h>
#include "lib/gth-histogram.h"
#include "lib/gth-image.h"
#include "lib/types.h"

#define MAP_ROWS 4
#define MAP_COLUMNS 256

void gth_image_apply_value_map (GthImage *self, long *value_map, int rows, int columns) {
	g_return_if_fail (rows == MAP_ROWS);
	g_return_if_fail (columns == MAP_COLUMNS);
	int row_stride;
	guint width;
	guint height;
	guchar *row = gth_image_prepare_edit (self, &row_stride, &width, &height);
	guchar *pixel;
	guchar red, green, blue, alpha;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	long *red_map = value_map + (GTH_CHANNEL_RED * MAP_COLUMNS);
	long *green_map = value_map + (GTH_CHANNEL_GREEN * MAP_COLUMNS);
	long *blue_map = value_map + (GTH_CHANNEL_BLUE * MAP_COLUMNS);
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
	}
}

static long* get_value_map_for_stretch (GthHistogram *histogram, double crop_size) {
	int n_pixels = gth_histogram_get_n_pixels (histogram);
	int lower_limit = (int) (crop_size * n_pixels);
	int higher_limit = (int) ((1.0 - crop_size) * n_pixels);
	long *value_map = g_new (long, MAP_ROWS * MAP_COLUMNS);
	int ofs = GTH_CHANNEL_RED * MAP_COLUMNS;
	for (int channel = GTH_CHANNEL_RED; channel <= GTH_CHANNEL_BLUE; channel++) {
		guchar min = 0;
		double sum = 0;
		for (int value = 0; value < 256; value++) {
			sum += gth_histogram_get_value (histogram, channel, value);
			if (sum >= lower_limit) {
				min = value;
				break;
			}
		}

		guchar max = 0;
		sum = 0;
		for (int value = 0; value < 256; value++) {
			sum += gth_histogram_get_value (histogram, channel, value);
			if (sum <= higher_limit) {
				max = value;
			}
		}

		long *channel_map = value_map + ofs;
		for (int value = 0; value <= min; value++) {
			channel_map[value] = 0;
		}

		double scale = 255.0 / (max - min);
		for (int value = min + 1; value < max; value++) {
			channel_map[value] = (int) round (scale * (value - min));
		}

		for (int value = max; value <= 255; value++) {
			channel_map[value] = 255;
		}
		ofs += MAP_COLUMNS;
	}
	return value_map;
}

void gth_image_stretch_histogram (GthImage *self, double crop_size) {
	GthHistogram *histogram = gth_histogram_new ();
	gth_histogram_update (histogram, self);

	long *value_map = get_value_map_for_stretch (histogram, crop_size);
	gth_image_apply_value_map (self, value_map, MAP_ROWS, MAP_COLUMNS);

	g_free (value_map);
	g_object_unref (histogram);
}

static double get_histogram_value (GthHistogram *histogram, GthChannel channel, int bin, gboolean linear) {
	double h = gth_histogram_get_value (histogram, channel, bin);
	return (linear || (h < 2)) ? h : sqrt (h);
}

static long* get_value_map_for_equalize (GthHistogram *histogram, gboolean linear) {
	long *value_map = g_new (long, MAP_ROWS * MAP_COLUMNS);
	int ofs = GTH_CHANNEL_RED * MAP_COLUMNS;
	for (int channel = GTH_CHANNEL_RED; channel <= GTH_CHANNEL_BLUE; channel++) {
		double sum = 0.0;
		for (int value = 0; value < 255; value++) {
			sum += 2 * get_histogram_value (histogram, channel, value, linear);
		}
		sum += get_histogram_value (histogram, channel, 255, linear);

		double scale = 255 / sum;

		long *channel_map = value_map + ofs;
		channel_map[0] = 0;
		sum = get_histogram_value (histogram, channel, 0, linear);
		for (int value = 1; value < 255; value++) {
			double delta = get_histogram_value (histogram, channel, value, linear);
			sum += delta;
			channel_map[value] = (int) round (sum * scale);
			sum += delta;
		}
		channel_map[255] = 255;
		ofs += MAP_COLUMNS;
	}
	return value_map;
}

void gth_image_equalize_histogram (GthImage *self, gboolean linear) {
	GthHistogram *histogram = gth_histogram_new ();
	gth_histogram_update (histogram, self);

	long *value_map = get_value_map_for_equalize (histogram, linear);
	gth_image_apply_value_map (self, value_map, MAP_ROWS, MAP_COLUMNS);

	g_free (value_map);
	g_object_unref (histogram);
}
