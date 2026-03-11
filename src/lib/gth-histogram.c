#include <config.h>
#include <glib.h>
#include "lib/gth-histogram.h"
#include "lib/pixel.h"

// Signals
enum {
	CHANGED,
	LAST_SIGNAL
};

struct _GthHistogramPrivate {
	int **values;
	int *values_max;
	int n_pixels;
	int n_channels;
	guchar *min_value;
	guchar *max_value;
};

static guint gth_histogram_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (GthHistogram, gth_histogram, G_TYPE_OBJECT, G_ADD_PRIVATE (GthHistogram))

static void gth_histogram_finalize (GObject *object) {
	GthHistogram *self = GTH_HISTOGRAM (object);

	for (int i = 0; i < GTH_HISTOGRAM_N_CHANNELS + 1; i++) {
		g_free (self->priv->values[i]);
	}
	g_free (self->priv->values);
	g_free (self->priv->values_max);
	g_free (self->priv->min_value);
	g_free (self->priv->max_value);

	G_OBJECT_CLASS (gth_histogram_parent_class)->finalize (object);
}

static void gth_histogram_class_init (GthHistogramClass *klass) {
	GObjectClass *object_class = (GObjectClass*) klass;
	object_class->finalize = gth_histogram_finalize;

	// signals

	gth_histogram_signals[CHANGED] =
		g_signal_new ("changed",
			G_TYPE_FROM_CLASS (klass),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (GthHistogramClass, changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);
}

static void gth_histogram_init (GthHistogram *self) {
	self->priv = gth_histogram_get_instance_private (self);
	self->priv->values = g_new0 (int *, GTH_HISTOGRAM_N_CHANNELS + 1);
	for (int i = 0; i < GTH_HISTOGRAM_N_CHANNELS + 1; i++) {
		self->priv->values[i] = g_new0 (int, 256);
	}
	self->priv->values_max = g_new0 (int, GTH_HISTOGRAM_N_CHANNELS + 1);
	self->priv->min_value = g_new0 (guchar, GTH_HISTOGRAM_N_CHANNELS + 1);
	self->priv->max_value = g_new0 (guchar, GTH_HISTOGRAM_N_CHANNELS + 1);
}

GthHistogram * gth_histogram_new (void) {
	return (GthHistogram *) g_object_new (GTH_TYPE_HISTOGRAM, NULL);
}

static void histogram_reset_values (GthHistogram *self) {
	for (int i = 0; i < GTH_HISTOGRAM_N_CHANNELS + 1; i++) {
		memset (self->priv->values[i], 0, sizeof (int) * 256);
		self->priv->values_max[i] = 0;
		self->priv->min_value[i] = 0;
		self->priv->max_value[i] = 0;
	}
}

static void gth_histogram_changed (GthHistogram *self) {
	g_signal_emit (self, gth_histogram_signals[CHANGED], 0);
}

void gth_histogram_update (GthHistogram *self, GthImage *image) {
	g_return_if_fail (GTH_IS_HISTOGRAM (self));

	if (image == NULL) {
		self->priv->n_channels = 0;
		histogram_reset_values (self);
		gth_histogram_changed (self);
		return;
	}

	gboolean has_alpha;
	if (!gth_image_get_has_alpha (image, &has_alpha)) {
		has_alpha = FALSE;
	}
	int rowstride, width, height;
	guchar *line = gth_image_prepare_edit (image, &rowstride, &width, &height);

	self->priv->n_pixels = width * height;
	self->priv->n_channels = (has_alpha ? 4 : 3) + 1;
	histogram_reset_values (self);

	int **values = self->priv->values;
	int *values_max = self->priv->values_max;
	guchar *min_value = self->priv->min_value;
	guchar *max_value = self->priv->max_value;
	guchar *pixel;
	int temp;
	guchar red, green, blue, alpha, value;
	for (int i = 0; i < height; i++) {
		pixel = line;
		for (int j = 0; j < width; j++) {
			PIXEL_TO_RGBA (pixel, red, green, blue, alpha);

			// Count values for each RGB channel

			values[GTH_CHANNEL_RED][red] += 1;
			values[GTH_CHANNEL_GREEN][green] += 1;
			values[GTH_CHANNEL_BLUE][blue] += 1;
			if (has_alpha) {
				values[GTH_CHANNEL_ALPHA][alpha] += 1;
			}

			// Count value for the Value channel

			value = MAX (MAX (red, green), blue);
			values[GTH_CHANNEL_VALUE][value] += 1;

			// Min and max pixel values

			min_value[GTH_CHANNEL_VALUE] = MIN (min_value[GTH_CHANNEL_VALUE], value);
			min_value[GTH_CHANNEL_RED] = MIN (min_value[GTH_CHANNEL_RED], red);
			min_value[GTH_CHANNEL_GREEN] = MIN (min_value[GTH_CHANNEL_GREEN], green);
			min_value[GTH_CHANNEL_BLUE] = MIN (min_value[GTH_CHANNEL_BLUE], blue);
			if (has_alpha) {
				min_value[GTH_CHANNEL_ALPHA] = MIN (min_value[GTH_CHANNEL_ALPHA], alpha);
			}

			max_value[GTH_CHANNEL_VALUE] = MAX (max_value[GTH_CHANNEL_VALUE], value);
			max_value[GTH_CHANNEL_RED] = MAX (max_value[GTH_CHANNEL_RED], red);
			max_value[GTH_CHANNEL_GREEN] = MAX (max_value[GTH_CHANNEL_GREEN], green);
			max_value[GTH_CHANNEL_BLUE] = MAX (max_value[GTH_CHANNEL_BLUE], blue);
			if (has_alpha) {
				max_value[GTH_CHANNEL_ALPHA] = MAX (max_value[GTH_CHANNEL_ALPHA], alpha);
			}

			// Track min and max value for each channel

			values_max[GTH_CHANNEL_VALUE] = MAX (values_max[GTH_CHANNEL_VALUE], values[GTH_CHANNEL_VALUE][value]);
			values_max[GTH_CHANNEL_RED] = MAX (values_max[GTH_CHANNEL_RED], values[GTH_CHANNEL_RED][red]);
			values_max[GTH_CHANNEL_GREEN] = MAX (values_max[GTH_CHANNEL_GREEN], values[GTH_CHANNEL_GREEN][green]);
			values_max[GTH_CHANNEL_BLUE] = MAX (values_max[GTH_CHANNEL_BLUE], values[GTH_CHANNEL_BLUE][blue]);
			if (has_alpha) {
				values_max[GTH_CHANNEL_ALPHA] = MAX (values_max[GTH_CHANNEL_ALPHA], values[GTH_CHANNEL_ALPHA][alpha]);
			}
			pixel += 4;
		}
		line += rowstride;
	}

	gth_histogram_changed (self);
}

double gth_histogram_get_count (GthHistogram *self, int start, int end) {
	g_return_val_if_fail (self != NULL, 0.0);
	double count = 0;
	for (int i = start; i <= end; i++) {
		count += self->priv->values[0][i];
	}
	return count;
}

double gth_histogram_get_value (GthHistogram *self, GthChannel channel, int bin) {
	g_return_val_if_fail (self != NULL, 0.0);
	if ((channel < self->priv->n_channels) && (bin >= 0) && (bin < 256)) {
		return (double) self->priv->values[channel][bin];
	}
	return 0.0;
}

double gth_histogram_get_channel_value (GthHistogram *self, GthChannel channel, int bin) {
	g_return_val_if_fail (self != NULL, 0.0);
	if (self->priv->n_channels > 3) {
		return gth_histogram_get_value (self, channel + 1, bin);
	}
	else {
		return gth_histogram_get_value (self, channel, bin);
	}
}

double gth_histogram_get_channel_max (GthHistogram *self, GthChannel channel) {
	g_return_val_if_fail (self != NULL, 0.0);
	if (channel < self->priv->n_channels) {
		return (double) self->priv->values_max[channel];
	}
	return 0.0;
}

double gth_histogram_get_max (GthHistogram *self) {
	g_return_val_if_fail (self != NULL, 0.0);
	double max = -1.0;
	for (int i = 0; i < self->priv->n_channels; i++) {
		max = MAX (max, (double) self->priv->values_max[i]);
	}
	return max;
}

guchar gth_histogram_get_min_value (GthHistogram *self, GthChannel channel) {
	g_return_val_if_fail (self != NULL, 0.0);
	if (channel < self->priv->n_channels) {
		return self->priv->min_value[channel];
	}
	return 0;
}

guchar gth_histogram_get_max_value (GthHistogram *self, GthChannel channel) {
	g_return_val_if_fail (self != NULL, 0.0);
	if (channel < self->priv->n_channels) {
		return self->priv->max_value[channel];
	}
	return 0;
}

int gth_histogram_get_n_pixels (GthHistogram *self) {
	g_return_val_if_fail (self != NULL, 0.0);
	return self->priv->n_pixels;
}

int gth_histogram_get_n_channels (GthHistogram *self) {
	g_return_val_if_fail (self != NULL, 0.0);
	return self->priv->n_channels - 1;
}

/*
long ** gth_histogram_get_cumulative (GthHistogram *self) {
	long **cumulative = g_new (long *, GTH_HISTOGRAM_N_CHANNELS);
	for (int c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		cumulative[c] = g_new (long, 256);
		int s = 0;
		for (int v = 0; v < 256; v++) {
			s += gth_histogram_get_value (self, c, v);
			cumulative[c][v] = s;
		}
	}
	return cumulative;
}

void gth_cumulative_histogram_free (long **cumulative) {
	for (int c = 0; c < GTH_HISTOGRAM_N_CHANNELS; c++) {
		g_free (cumulative[c]);
	}
	g_free (cumulative);
}
*/
