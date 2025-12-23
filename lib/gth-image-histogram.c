#include <config.h>
#include <math.h>
#include "lib/gth-histogram.h"
#include "lib/gth-image.h"
#include "lib/types.h"

static guchar* get_value_map_for_stretch (GthHistogram *histogram, double crop_size) {
	int n_pixels = gth_histogram_get_n_pixels (histogram);
	int lower_limit = (int) (crop_size * n_pixels);
	int higher_limit = (int) ((1.0 - crop_size) * n_pixels);
	guchar *value_map = g_new (guchar, VALUE_MAP_ROWS * VALUE_MAP_COLUMNS);
	int ofs = GTH_CHANNEL_RED * VALUE_MAP_COLUMNS;
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

		guchar *channel_map = value_map + ofs;
		for (int value = 0; value <= min; value++) {
			channel_map[value] = 0;
		}

		double scale = 255.0 / (max - min);
		for (int value = min + 1; value < max; value++) {
			channel_map[value] = (guchar) round (scale * (value - min));
		}

		for (int value = max; value <= 255; value++) {
			channel_map[value] = 255;
		}
		ofs += VALUE_MAP_COLUMNS;
	}
	return value_map;
}

gboolean gth_image_stretch_histogram (GthImage *self, double crop_size, GCancellable *cancellable) {
	GthHistogram *histogram = gth_histogram_new ();
	gth_histogram_update (histogram, self);

	guchar *value_map = get_value_map_for_stretch (histogram, crop_size);
	gboolean completed = gth_image_apply_value_map (self, value_map, cancellable);

	g_free (value_map);
	g_object_unref (histogram);

	return completed;
}

static double get_histogram_value (GthHistogram *histogram, GthChannel channel, int bin, gboolean linear) {
	double h = gth_histogram_get_value (histogram, channel, bin);
	return (linear || (h < 2)) ? h : sqrt (h);
}

static guchar* get_value_map_for_equalize (GthHistogram *histogram, gboolean linear) {
	guchar *value_map = g_new (guchar, VALUE_MAP_ROWS * VALUE_MAP_COLUMNS);
	int ofs = GTH_CHANNEL_RED * VALUE_MAP_COLUMNS;
	for (int channel = GTH_CHANNEL_RED; channel <= GTH_CHANNEL_BLUE; channel++) {
		double sum = 0.0;
		for (int value = 0; value < 255; value++) {
			sum += 2 * get_histogram_value (histogram, channel, value, linear);
		}
		sum += get_histogram_value (histogram, channel, 255, linear);

		double scale = 255 / sum;

		guchar *channel_map = value_map + ofs;
		channel_map[0] = 0;
		sum = get_histogram_value (histogram, channel, 0, linear);
		for (int value = 1; value < 255; value++) {
			double delta = get_histogram_value (histogram, channel, value, linear);
			sum += delta;
			channel_map[value] = (guchar) round (sum * scale);
			sum += delta;
		}
		channel_map[255] = 255;
		ofs += VALUE_MAP_COLUMNS;
	}
	return value_map;
}

gboolean gth_image_equalize_histogram (GthImage *self, gboolean linear, GCancellable *cancellable) {
	GthHistogram *histogram = gth_histogram_new ();
	gth_histogram_update (histogram, self);

	guchar *value_map = get_value_map_for_equalize (histogram, linear);
	gboolean completed = gth_image_apply_value_map (self, value_map, cancellable);

	g_free (value_map);
	g_object_unref (histogram);

	return completed;
}
