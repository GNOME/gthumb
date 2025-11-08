#include <config.h>
#include <math.h>
#include "lib/gth-histogram.h"
#include "lib/gth-image.h"
#include "lib/types.h"

static long ** get_value_map_for_stretch (GthHistogram *histogram, double crop_size) {
	int n_pixels = gth_histogram_get_n_pixels (histogram);
	int lower_limit = (int) (crop_size * n_pixels);
	int higher_limit = (int) ((1.0 - crop_size) * n_pixels);
	long **value_map = g_new (long *, GTH_HISTOGRAM_N_CHANNELS);
	for (int channel = 0; channel < GTH_HISTOGRAM_N_CHANNELS; channel++) {
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

		value_map[channel] = g_new (long, 256);
		for (int value = 0; value <= min; value++) {
			value_map[channel][value] = 0;
		}

		double scale = 255.0 / (max - min);
		for (int value = min + 1; value < max; value++) {
			value_map[channel][value] = (int) round (scale * (value - min));
		}

		for (int value = max; value <= 255; value++) {
			value_map[channel][value] = 255;
		}
	}
	return value_map;
}

static void value_map_free (long **value_map) {
	for (uint channel = 0; channel < GTH_HISTOGRAM_N_CHANNELS; channel++) {
		g_free (value_map[channel]);
	}
	g_free (value_map);
}

static void apply_value_map (GthImage *self, long **value_map) {
	int row_stride;
	guint width;
	guint height;
	guchar *row = gth_image_prepare_edit (self, &row_stride, &width, &height);
	guchar *pixel;
	guchar red, green, blue, alpha;
	guchar r, g, b; // used in RGBA_TO_PIXEL
	guint temp; // used in RGBA_TO_PIXEL
	for (guint y = 0; y < height; y++) {
		pixel = row;
		for (guint x = 0; x < width; x++) {
			PIXEL_TO_RGBA (pixel, red, green, blue, alpha);
			red = value_map[GTH_CHANNEL_RED][red];
			green = value_map[GTH_CHANNEL_GREEN][green];
			blue = value_map[GTH_CHANNEL_BLUE][blue];
			RGBA_TO_PIXEL (pixel, red, green, blue, alpha);
			pixel += 4;
		}
		row += row_stride;
	}
}

void gth_image_stretch_histogram (GthImage *self, double crop_size) {
	GthHistogram *histogram = gth_histogram_new ();
	gth_histogram_calculate (histogram, self);
	long **value_map = get_value_map_for_stretch (histogram, crop_size);
	apply_value_map (self, value_map);

	value_map_free (value_map);
	g_object_unref (histogram);
}

static double get_histogram_value (GthHistogram *histogram, GthChannel channel, int bin, gboolean linear) {
	double h = gth_histogram_get_value (histogram, channel, bin);
	return (linear || (h < 2)) ? h : sqrt (h);
}

static long ** get_value_map_for_equalize (GthHistogram *histogram, gboolean linear) {
	long **value_map;
	value_map = g_new (long *, GTH_HISTOGRAM_N_CHANNELS);
	for (int channel = 0; channel < GTH_HISTOGRAM_N_CHANNELS; channel++) {
		double sum = 0.0;
		for (int value = 0; value < 255; value++) {
			sum += 2 * get_histogram_value (histogram, channel, value, linear);
		}
		sum += get_histogram_value (histogram, channel, 255, linear);

		double scale = 255 / sum;

		value_map[channel] = g_new (long, 256);
		value_map[channel][0] = 0;
		sum = get_histogram_value (histogram, channel, 0, linear);
		for (int value = 1; value < 255; value++) {
			double delta = get_histogram_value (histogram, channel, value, linear);
			sum += delta;
			value_map[channel][value] = (int) round (sum * scale);
			sum += delta;
		}
		value_map[channel][255] = 255;
	}
	return value_map;
}

void gth_image_equalize_histogram (GthImage *self, gboolean linear) {
	GthHistogram *histogram = gth_histogram_new ();
	gth_histogram_calculate (histogram, self);
	long **value_map = get_value_map_for_equalize (histogram, linear);
	apply_value_map (self, value_map);

	value_map_free (value_map);
	g_object_unref (histogram);
}
