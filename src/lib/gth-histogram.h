#ifndef GTH_HISTOGRAM_H
#define GTH_HISTOGRAM_H

#include <glib-object.h>
#include "lib/gth-image.h"
#include "lib/types.h"

G_BEGIN_DECLS

#define GTH_TYPE_HISTOGRAM (gth_histogram_get_type ())
#define GTH_HISTOGRAM(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GTH_TYPE_HISTOGRAM, GthHistogram))
#define GTH_HISTOGRAM_CLASS(k) (G_TYPE_CHECK_CLASS_CAST ((k), GTH_TYPE_HISTOGRAM, GthHistogramClass))
#define GTH_IS_HISTOGRAM(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GTH_TYPE_HISTOGRAM))
#define GTH_IS_HISTOGRAM_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GTH_TYPE_HISTOGRAM))
#define GTH_HISTOGRAM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GTH_TYPE_HISTOGRAM, GthHistogramClass))

typedef struct _GthHistogram GthHistogram;
typedef struct _GthHistogramPrivate GthHistogramPrivate;
typedef struct _GthHistogramClass GthHistogramClass;

struct _GthHistogram {
	GObject __parent;
	GthHistogramPrivate *priv;
};

struct _GthHistogramClass {
	GObjectClass __parent_class;
	void (*changed) (GthHistogram *self);
};

GType gth_histogram_get_type (void) G_GNUC_CONST;
GthHistogram * gth_histogram_new (void);
void gth_histogram_update (GthHistogram *self, GthImage *image);
double gth_histogram_get_count (GthHistogram *self, int start, int end);
double gth_histogram_get_value (GthHistogram *self, GthChannel channel, int bin);
double gth_histogram_get_channel_value (GthHistogram *self, GthChannel channel, int bin);
double gth_histogram_get_channel_max (GthHistogram *self, GthChannel channel);
double gth_histogram_get_max (GthHistogram *self);
guchar gth_histogram_get_min_value (GthHistogram *self, GthChannel channel);
guchar gth_histogram_get_max_value (GthHistogram *self, GthChannel channel);
int gth_histogram_get_n_pixels (GthHistogram *self);
int gth_histogram_get_n_channels (GthHistogram *self);

#endif /* GTH_HISTOGRAM_H */
